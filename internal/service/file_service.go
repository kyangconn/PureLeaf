package service

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
)

// FileService 文件管理与编译业务接口
type FileService interface {
	GetTree(projectID uint) ([]*domain.File, error)
	GetContent(fileID, projectID uint) (*domain.File, error)
	Create(projectID uint, name string, parentID *uint, isDir bool) (*domain.File, error)
	UpdateContent(fileID, projectID uint, content string) (*domain.File, error)
	Rename(fileID, projectID uint, newName string) (*domain.File, error)
	Delete(fileID, projectID uint) error
	Compile(projectID uint) (pdfPath, logOutput string, err error)

	// 历史与快照
	GetFileHistory(fileID, projectID uint, limit int) ([]domain.FileRevision, error)
	GetProjectHistory(projectID uint, limit int) ([]domain.FileRevision, error)
	GetProjectSnapshots(projectID uint, limit int) ([]domain.ProjectSnapshot, error)
	GetRevisionContent(revID uint) (string, error)
	DiffRevisions(revA, revB uint) (string, error)

	// 系统
	OpenProjectFolder(projectID uint) error
}

type fileService struct {
	fileRepo    repository.FileRepository
	projectRepo repository.ProjectRepository
	revRepo     repository.FileRevisionRepository
	snapRepo    repository.ProjectSnapshotRepository
	lockManager ProjectLockManager
	blobStore   BlobStore
	compiler    string
	compileTO   int
	dataDir     string // data/projects — 项目文件根目录
}

// NewFileService 创建文件服务
func NewFileService(
	fileRepo repository.FileRepository,
	projectRepo repository.ProjectRepository,
	revRepo repository.FileRevisionRepository,
	snapRepo repository.ProjectSnapshotRepository,
	lockManager ProjectLockManager,
	blobStore BlobStore,
	compiler string, timeout int, dataDir string,
) FileService {
	return &fileService{
		fileRepo:    fileRepo,
		projectRepo: projectRepo,
		revRepo:     revRepo,
		snapRepo:    snapRepo,
		lockManager: lockManager,
		blobStore:   blobStore,
		compiler:    compiler,
		compileTO:   timeout,
		dataDir:     dataDir,
	}
}

// ---- 文件树 ----

func (s *fileService) GetTree(projectID uint) ([]*domain.File, error) {
	var result []*domain.File
	err := s.lockManager.WithProjectLock(projectID, func() error {
		files, err := s.fileRepo.FindByProjectID(projectID)
		if err != nil {
			return err
		}
		result = buildTree(files, nil)
		return nil
	})
	return result, err
}

func (s *fileService) GetContent(fileID, projectID uint) (*domain.File, error) {
	var file *domain.File
	err := s.lockManager.WithProjectLock(projectID, func() error {
		var err error
		file, err = s.fileRepo.FindByID(fileID)
		if err != nil {
			return err
		}
		if file.ProjectID != projectID {
			return repository.ErrFileNotFound
		}
		if file.IsDir {
			return nil
		}

		relPath, err := s.computePath(fileID, projectID)
		if err != nil {
			return err
		}
		if err := ValidateRelativePath(relPath); err != nil {
			return err
		}
		fullPath := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID), relPath)
		data, err := os.ReadFile(fullPath)
		if err != nil {
			return fmt.Errorf("读取文件内容失败: %w", err)
		}
		file.Content = string(data)
		return nil
	})
	return file, err
}

// ---- 增删改 ----

func (s *fileService) Create(projectID uint, name string, parentID *uint, isDir bool) (*domain.File, error) {
	if err := ValidateFileName(name); err != nil {
		return nil, err
	}
	var file *domain.File
	err := s.lockManager.WithProjectLock(projectID, func() error {
		// 同级重名检查（含文件与文件夹）
		if existing, lookupErr := s.fileRepo.FindByNameInParent(projectID, parentID, name, 0); lookupErr == nil && existing != nil {
			return ErrNameConflict
		} else if lookupErr != nil && !errors.Is(lookupErr, repository.ErrFileNotFound) {
			return lookupErr
		}

		file = &domain.File{ProjectID: projectID, ParentID: parentID, Name: name, IsDir: isDir}
		if err := s.fileRepo.Create(file); err != nil {
			return err
		}

		// 写入磁盘
		relPath, err := s.computePath(file.ID, projectID)
		if err != nil {
			return err
		}
		if err := ValidateRelativePath(relPath); err != nil {
			return err
		}
		fullPath := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID), relPath)
		if isDir {
			if err := os.MkdirAll(fullPath, 0755); err != nil {
				return err
			}
		} else {
			if err := writeFileAtomic(fullPath, []byte(""), 0644); err != nil {
				return err
			}
		}
		return nil
	})
	return file, err
}

func (s *fileService) UpdateContent(fileID, projectID uint, content string) (*domain.File, error) {
	var file *domain.File
	err := s.lockManager.WithProjectLock(projectID, func() error {
		var err error
		file, err = s.fileRepo.FindByID(fileID)
		if err != nil {
			return err
		}
		if file.ProjectID != projectID {
			return repository.ErrFileNotFound
		}
		if file.IsDir {
			return repository.ErrFileNotFound
		}

		relPath, err := s.computePath(fileID, projectID)
		if err != nil {
			return err
		}
		if err := ValidateRelativePath(relPath); err != nil {
			return err
		}
		fullPath := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID), relPath)
		if err := writeFileAtomic(fullPath, []byte(content), 0644); err != nil {
			return err
		}
		file.Content = content

		// 记录文件历史：内容寻址 blob 去重，hash 未变则跳过。
		if s.blobStore != nil && s.revRepo != nil {
			hash, blobPath, size, putErr := s.blobStore.Put([]byte(content))
			if putErr != nil {
				// 历史失败不阻断主保存流程，文件本身已落盘。
				return nil
			}
			if latest, lookupErr := s.revRepo.LatestByFileID(fileID); lookupErr == nil && latest != nil && latest.ContentHash == hash {
				// 内容与上一个版本一致，跳过重复记录。
				return nil
			}
			_ = s.revRepo.Create(&domain.FileRevision{
				ProjectID:   projectID,
				FileID:      fileID,
				FilePath:    relPath,
				ContentHash: hash,
				BlobPath:    blobPath,
				Size:        size,
				Reason:      RevisionReasonSave,
			})
		}
		return nil
	})
	return file, err
}

func (s *fileService) Rename(fileID, projectID uint, newName string) (*domain.File, error) {
	if err := ValidateFileName(newName); err != nil {
		return nil, err
	}
	var file *domain.File
	err := s.lockManager.WithProjectLock(projectID, func() error {
		var err error
		file, err = s.fileRepo.FindByID(fileID)
		if err != nil {
			return err
		}
		if file.ProjectID != projectID {
			return repository.ErrFileNotFound
		}
		if file.Name == newName {
			return nil // 无变化
		}

		// 同级重名检查（排除自身）
		if existing, lookupErr := s.fileRepo.FindByNameInParent(projectID, file.ParentID, newName, fileID); lookupErr == nil && existing != nil {
			return ErrNameConflict
		} else if lookupErr != nil && !errors.Is(lookupErr, repository.ErrFileNotFound) {
			return lookupErr
		}

		// 算出旧路径
		oldRel, err := s.computePath(fileID, projectID)
		if err != nil {
			return err
		}
		if err := ValidateRelativePath(oldRel); err != nil {
			return err
		}

		// 更新 DB 名称
		file.Name = newName
		if err := s.fileRepo.Update(file); err != nil {
			return err
		}

		// 算新路径并重命名磁盘文件
		newRel, err := s.computePath(fileID, projectID)
		if err != nil {
			return err
		}
		if err := ValidateRelativePath(newRel); err != nil {
			return err
		}
		baseDir := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID))
		oldPath := filepath.Join(baseDir, oldRel)
		newPath := filepath.Join(baseDir, newRel)
		if err := os.Rename(oldPath, newPath); err != nil {
			return err
		}
		return nil
	})
	return file, err
}

func (s *fileService) Delete(fileID, projectID uint) error {
	return s.lockManager.WithProjectLock(projectID, func() error {
		// 校验所属项目
		target, err := s.fileRepo.FindByID(fileID)
		if err != nil {
			return err
		}
		if target.ProjectID != projectID {
			return repository.ErrFileNotFound
		}

		// 算出路径
		relPath, err := s.computePath(fileID, projectID)
		if err != nil {
			return err
		}
		if err := ValidateRelativePath(relPath); err != nil {
			return err
		}

		// 递归收集子孙 ID
		ids, err := s.collectDescendants(projectID, fileID)
		if err != nil {
			return err
		}
		ids = append(ids, fileID)

		// 删除前快照：把将被删除的文件内容落入 blob 并记录版本。
		if snapshotErr := s.snapshotBeforeDelete(projectID, ids, relPath); snapshotErr != nil {
			// 快照失败不阻断删除（安全网尽量保留，但不掣肘主流程）。
			_ = snapshotErr
		}

		// 删 DB 记录
		if err := s.fileRepo.DeleteByIDs(ids); err != nil {
			return err
		}

		// 删磁盘
		fullPath := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID), relPath)
		return os.RemoveAll(fullPath)
	})
}

// snapshotBeforeDelete 为即将被删除的文件集生成项目快照。
// 创建一条 project_snapshots 记录，并为每个文件写一条关联的 file_revisions。
func (s *fileService) snapshotBeforeDelete(projectID uint, fileIDs []uint, rootPath string) error {
	if s.blobStore == nil || s.revRepo == nil || s.snapRepo == nil {
		return nil
	}
	// 收集所有被删文件（非目录）及其磁盘路径
	baseDir := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID))
	var (
		revs    []domain.FileRevision
		totalSz int64
		count   int
	)
	for _, fid := range fileIDs {
		f, err := s.fileRepo.FindByID(fid)
		if err != nil || f.IsDir {
			continue
		}
		fRel, err := s.computePath(fid, projectID)
		if err != nil {
			continue
		}
		if err := ValidateRelativePath(fRel); err != nil {
			continue
		}
		data, err := os.ReadFile(filepath.Join(baseDir, fRel))
		if err != nil {
			continue // 磁盘缺失的文件跳过，不阻断快照
		}
		hash, blobPath, size, putErr := s.blobStore.Put(data)
		if putErr != nil {
			continue
		}
		revs = append(revs, domain.FileRevision{
			ProjectID:   projectID,
			FileID:      fid,
			FilePath:    fRel,
			ContentHash: hash,
			BlobPath:    blobPath,
			Size:        size,
			Reason:      RevisionReasonDelete,
		})
		totalSz += size
		count++
	}
	if count == 0 {
		return nil // 没有可快照的文件内容
	}

	snap := &domain.ProjectSnapshot{
		ProjectID:  projectID,
		Reason:     SnapshotReasonDelete,
		FileCount:  count,
		TotalSize:  totalSz,
		SnapshotOf: rootPath,
	}
	if err := s.snapRepo.Create(snap); err != nil {
		return err
	}
	for i := range revs {
		revs[i].SnapshotID = &snap.ID
		_ = s.revRepo.Create(&revs[i])
	}
	return nil
}

func (s *fileService) collectDescendants(projectID, parentID uint) ([]uint, error) {
	children, err := s.fileRepo.FindByParentID(projectID, parentID)
	if err != nil {
		return nil, err
	}
	var ids []uint
	for _, c := range children {
		ids = append(ids, c.ID)
		sub, _ := s.collectDescendants(projectID, c.ID)
		ids = append(ids, sub...)
	}
	return ids, nil
}

// ---- 编译 ----

func (s *fileService) Compile(projectID uint) (string, string, error) {
	var pdfPath string
	var logOutput string
	err := s.lockManager.WithProjectLock(projectID, func() error {
		// 找主文件
		mainFile, err := s.findMainFile(projectID)
		if err != nil {
			return err
		}

		// 文件已在磁盘，直接以项目目录为工作目录
		workDir := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID))

		// 运行编译器
		ctx, cancel := context.WithTimeout(context.Background(), time.Duration(s.compileTO)*time.Second)
		defer cancel()

		cmd := exec.CommandContext(ctx, s.compiler,
			"-interaction=nonstopmode",
			"-output-directory="+workDir,
			mainFile,
		)
		cmd.Dir = workDir

		var stdout, stderr bytes.Buffer
		cmd.Stdout = &stdout
		cmd.Stderr = &stderr

		runErr := cmd.Run()
		logOutput = stdout.String()
		if stderr.Len() > 0 {
			logOutput += "\n[stderr]\n" + stderr.String()
		}

		if runErr != nil {
			if ctx.Err() == context.DeadlineExceeded {
				return fmt.Errorf("编译超时")
			}
		}

		// 查找 PDF
		pdfName := strings.TrimSuffix(mainFile, ".tex") + ".pdf"
		pdfPath = filepath.Join(workDir, pdfName)
		if _, statErr := os.Stat(pdfPath); os.IsNotExist(statErr) {
			return fmt.Errorf("编译失败，未生成 PDF:\n%s", logOutput)
		}

		return nil
	})
	return pdfPath, logOutput, err
}

func (s *fileService) findMainFile(projectID uint) (string, error) {
	files, err := s.fileRepo.FindTeXFiles(projectID)
	if err != nil {
		return "", err
	}
	if len(files) == 0 {
		return "", fmt.Errorf("项目中没有任何 .tex 文件")
	}
	for _, f := range files {
		if strings.EqualFold(f.Name, "main.tex") {
			return f.Name, nil
		}
	}
	for _, f := range files {
		if strings.HasSuffix(strings.ToLower(f.Name), ".tex") {
			return f.Name, nil
		}
	}
	return "", fmt.Errorf("项目中找不到 .tex 文件")
}

// computePath 根据 fileID 和 projectID 计算文件在项目内的相对路径。
// 结果会经过 ValidateRelativePath 校验，防止历史非法数据造成路径逃逸。
func (s *fileService) computePath(fileID, projectID uint) (string, error) {
	files, err := s.fileRepo.FindByProjectID(projectID)
	if err != nil {
		return "", err
	}
	fileMap := make(map[uint]*domain.File)
	for i := range files {
		fileMap[files[i].ID] = &files[i]
	}

	var parts []string
	currentID := &fileID
	for {
		f, ok := fileMap[*currentID]
		if !ok {
			break
		}
		parts = append([]string{f.Name}, parts...)
		if f.ParentID == nil {
			break
		}
		currentID = f.ParentID
	}
	rel := filepath.Join(parts...)
	if err := ValidateRelativePath(rel); err != nil {
		return "", err
	}
	return rel, nil
}

// buildTree 扁平列表 → 树
func buildTree(files []domain.File, parentID *uint) []*domain.File {
	var result []*domain.File
	for i := range files {
		f := &files[i]
		if (parentID == nil && f.ParentID == nil) || (parentID != nil && f.ParentID != nil && *f.ParentID == *parentID) {
			f.Children = buildTree(files, &f.ID)
			result = append(result, f)
		}
	}
	return result
}

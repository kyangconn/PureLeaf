package service

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
)

// 默认 main.tex 模板
const defaultMainTex = `\documentclass{article}
\usepackage[UTF8]{ctex}

\title{未命名文档}
\author{}

\begin{document}
\maketitle

\section{开始写作}
欢迎使用 goleaf！

\end{document}
`

// ProjectService 项目业务接口
type ProjectService interface {
	Create(name string) (*domain.Project, error)
	GetByID(projectID uint) (*domain.Project, error)
	List() ([]domain.Project, error)
	Update(projectID uint, name string) (*domain.Project, error)
	Delete(projectID uint) error
}

type projectService struct {
	projectRepo repository.ProjectRepository
	fileRepo    repository.FileRepository
	revRepo     repository.FileRevisionRepository
	snapRepo    repository.ProjectSnapshotRepository
	lockManager ProjectLockManager
	blobStore   BlobStore
	outputDir   string
}

// NewProjectService 创建项目服务
func NewProjectService(
	projectRepo repository.ProjectRepository,
	fileRepo repository.FileRepository,
	revRepo repository.FileRevisionRepository,
	snapRepo repository.ProjectSnapshotRepository,
	lockManager ProjectLockManager,
	blobStore BlobStore,
	outputDir string,
) ProjectService {
	return &projectService{
		projectRepo: projectRepo,
		fileRepo:    fileRepo,
		revRepo:     revRepo,
		snapRepo:    snapRepo,
		lockManager: lockManager,
		blobStore:   blobStore,
		outputDir:   outputDir,
	}
}

func (s *projectService) Create(name string) (*domain.Project, error) {
	project := &domain.Project{Name: name}
	if err := s.projectRepo.Create(project); err != nil {
		return nil, err
	}

	// main.tex 元数据
	mainTex := &domain.File{
		ProjectID: project.ID,
		Name:      "main.tex",
		IsDir:     false,
	}
	if err := s.fileRepo.Create(mainTex); err != nil {
		return nil, err
	}

	// 写模板到磁盘
	projDir := filepath.Join(s.outputDir, fmt.Sprintf("%d", project.ID))
	if err := os.MkdirAll(projDir, 0755); err != nil {
		// 补偿：删除 DB 记录
		s.fileRepo.Delete(mainTex.ID)
		s.projectRepo.Delete(project.ID)
		return nil, fmt.Errorf("创建项目目录失败: %w", err)
	}
	if err := writeFileAtomic(filepath.Join(projDir, "main.tex"), []byte(defaultMainTex), 0644); err != nil {
		os.RemoveAll(projDir)
		s.fileRepo.Delete(mainTex.ID)
		s.projectRepo.Delete(project.ID)
		return nil, fmt.Errorf("写入模板文件失败: %w", err)
	}

	return project, nil
}

func (s *projectService) GetByID(projectID uint) (*domain.Project, error) {
	return s.projectRepo.FindByID(projectID)
}

func (s *projectService) List() ([]domain.Project, error) {
	return s.projectRepo.FindAll()
}

func (s *projectService) Update(projectID uint, name string) (*domain.Project, error) {
	var project *domain.Project
	err := s.lockManager.WithProjectLock(projectID, func() error {
		var err error
		project, err = s.projectRepo.FindByID(projectID)
		if err != nil {
			return err
		}
		project.Name = name
		return s.projectRepo.Update(project)
	})
	return project, err
}

func (s *projectService) Delete(projectID uint) error {
	return s.lockManager.WithProjectLock(projectID, func() error {
		if _, err := s.projectRepo.FindByID(projectID); err != nil {
			return err
		}

		// 删除前为整个项目生成快照：把项目内所有文件内容落入 blob 并记录版本。
		if snapErr := s.snapshotProjectFiles(projectID); snapErr != nil {
			_ = snapErr // 快照失败不阻断删除
		}

		if err := s.fileRepo.DeleteByProjectID(projectID); err != nil {
			return err
		}
		if err := s.projectRepo.Delete(projectID); err != nil {
			return err
		}

		// 清理磁盘上的项目文件
		projDir := filepath.Join(s.outputDir, fmt.Sprintf("%d", projectID))
		_ = os.RemoveAll(projDir)

		return nil
	})
}

// snapshotProjectFiles 为项目内所有文件生成项目快照。
func (s *projectService) snapshotProjectFiles(projectID uint) error {
	if s.blobStore == nil || s.revRepo == nil || s.snapRepo == nil {
		return nil
	}
	files, err := s.fileRepo.FindByProjectID(projectID)
	if err != nil {
		return err
	}
	// 构建 id -> file 映射，用于计算嵌套文件的相对路径。
	fileMap := make(map[uint]*domain.File, len(files))
	for i := range files {
		fileMap[files[i].ID] = &files[i]
	}
	baseDir := filepath.Join(s.outputDir, fmt.Sprintf("%d", projectID))
	var (
		revs    []domain.FileRevision
		totalSz int64
		count   int
	)
	for i := range files {
		f := files[i]
		if f.IsDir {
			continue
		}
		rel := computeFilePath(fileMap, f.ID)
		if err := ValidateRelativePath(rel); err != nil {
			continue
		}
		data, readErr := os.ReadFile(filepath.Join(baseDir, rel))
		if readErr != nil {
			continue
		}
		hash, blobPath, size, putErr := s.blobStore.Put(data)
		if putErr != nil {
			continue
		}
		revs = append(revs, domain.FileRevision{
			ProjectID:   projectID,
			FileID:      f.ID,
			FilePath:    rel,
			ContentHash: hash,
			BlobPath:    blobPath,
			Size:        size,
			Reason:      RevisionReasonDelete,
		})
		totalSz += size
		count++
	}
	if count == 0 {
		return nil
	}
	snap := &domain.ProjectSnapshot{
		ProjectID:  projectID,
		Reason:     SnapshotReasonDelete,
		FileCount:  count,
		TotalSize:  totalSz,
		SnapshotOf: fmt.Sprintf("project/%d", projectID),
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

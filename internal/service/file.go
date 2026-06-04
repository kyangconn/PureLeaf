package service

import (
	"bytes"
	"context"
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
	GetTree(projectID, userID uint) ([]*domain.File, error)
	GetContent(fileID, projectID, userID uint) (*domain.File, error)
	Create(projectID, userID uint, name string, parentID *uint, isDir bool) (*domain.File, error)
	UpdateContent(fileID, projectID, userID uint, content string) (*domain.File, error)
	Rename(fileID, projectID, userID uint, newName string) (*domain.File, error)
	Delete(fileID, projectID, userID uint) error
	Compile(projectID, userID uint) (pdfPath, logOutput string, err error)
}

type fileService struct {
	fileRepo    repository.FileRepository
	projectRepo repository.ProjectRepository
	compiler    string
	compileTO   int
	outputDir   string
}

// NewFileService 创建文件服务
func NewFileService(
	fileRepo repository.FileRepository,
	projectRepo repository.ProjectRepository,
	compiler string, timeout int, outputDir string,
) FileService {
	return &fileService{
		fileRepo:    fileRepo,
		projectRepo: projectRepo,
		compiler:    compiler,
		compileTO:   timeout,
		outputDir:   outputDir,
	}
}

func (s *fileService) checkAccess(projectID, userID uint) error {
	project, err := s.projectRepo.FindByID(projectID)
	if err != nil {
		return err
	}
	if project.OwnerID == userID {
		return nil
	}
	if ok, _ := s.projectRepo.IsCollaborator(projectID, userID); ok {
		return nil
	}
	return repository.ErrForbidden
}

// ---- 文件树 ----

func (s *fileService) GetTree(projectID, userID uint) ([]*domain.File, error) {
	if err := s.checkAccess(projectID, userID); err != nil {
		return nil, err
	}
	files, err := s.fileRepo.FindByProjectID(projectID)
	if err != nil {
		return nil, err
	}
	return buildTree(files, nil), nil
}

func (s *fileService) GetContent(fileID, projectID, userID uint) (*domain.File, error) {
	if err := s.checkAccess(projectID, userID); err != nil {
		return nil, err
	}
	file, err := s.fileRepo.FindByID(fileID)
	if err != nil {
		return nil, err
	}
	if file.ProjectID != projectID {
		return nil, repository.ErrFileNotFound
	}
	return file, nil
}

// ---- 增删改 ----

func (s *fileService) Create(projectID, userID uint, name string, parentID *uint, isDir bool) (*domain.File, error) {
	if err := s.checkAccess(projectID, userID); err != nil {
		return nil, err
	}
	file := &domain.File{ProjectID: projectID, ParentID: parentID, Name: name, IsDir: isDir}
	if err := s.fileRepo.Create(file); err != nil {
		return nil, err
	}
	return file, nil
}

func (s *fileService) UpdateContent(fileID, projectID, userID uint, content string) (*domain.File, error) {
	if err := s.checkAccess(projectID, userID); err != nil {
		return nil, err
	}
	file, err := s.fileRepo.FindByID(fileID)
	if err != nil {
		return nil, err
	}
	if file.IsDir {
		return nil, repository.ErrFileNotFound
	}
	file.Content = content
	if err := s.fileRepo.Update(file); err != nil {
		return nil, err
	}
	return file, nil
}

func (s *fileService) Rename(fileID, projectID, userID uint, newName string) (*domain.File, error) {
	if err := s.checkAccess(projectID, userID); err != nil {
		return nil, err
	}
	file, err := s.fileRepo.FindByID(fileID)
	if err != nil {
		return nil, err
	}
	file.Name = newName
	if err := s.fileRepo.Update(file); err != nil {
		return nil, err
	}
	return file, nil
}

func (s *fileService) Delete(fileID, projectID, userID uint) error {
	if err := s.checkAccess(projectID, userID); err != nil {
		return err
	}
	// 递归收集子孙 ID
	ids, err := s.collectDescendants(projectID, fileID)
	if err != nil {
		return err
	}
	ids = append(ids, fileID)
	return s.fileRepo.DeleteByIDs(ids)
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

func (s *fileService) Compile(projectID, userID uint) (string, string, error) {
	if err := s.checkAccess(projectID, userID); err != nil {
		return "", "", err
	}

	// 找主文件
	mainFile, err := s.findMainFile(projectID)
	if err != nil {
		return "", "", err
	}

	// 编译工作目录
	workDir := filepath.Join(s.outputDir, fmt.Sprintf("%d", projectID))
	if err := os.MkdirAll(workDir, 0755); err != nil {
		return "", "", err
	}

	// 写入所有文件
	if err := s.writeFiles(projectID, workDir); err != nil {
		return "", "", err
	}

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
	logOutput := stdout.String()
	if stderr.Len() > 0 {
		logOutput += "\n[stderr]\n" + stderr.String()
	}

	if runErr != nil {
		if ctx.Err() == context.DeadlineExceeded {
			return "", logOutput, fmt.Errorf("编译超时")
		}
	}

	// 查找 PDF
	pdfName := strings.TrimSuffix(mainFile, ".tex") + ".pdf"
	pdfPath := filepath.Join(workDir, pdfName)
	if _, statErr := os.Stat(pdfPath); os.IsNotExist(statErr) {
		return "", logOutput, fmt.Errorf("编译失败，未生成 PDF:\n%s", logOutput)
	}

	return pdfPath, logOutput, nil
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

func (s *fileService) writeFiles(projectID uint, baseDir string) error {
	files, err := s.fileRepo.FindByProjectID(projectID)
	if err != nil {
		return err
	}

	// 构建 ID → File 映射
	fileMap := make(map[uint]*domain.File)
	for i := range files {
		fileMap[files[i].ID] = &files[i]
	}

	for _, f := range files {
		if f.IsDir {
			continue
		}
		relPath := s.buildPath(f.ID, fileMap)
		fullPath := filepath.Join(baseDir, relPath)
		if err := os.MkdirAll(filepath.Dir(fullPath), 0755); err != nil {
			return err
		}
		if err := os.WriteFile(fullPath, []byte(f.Content), 0644); err != nil {
			return err
		}
	}
	return nil
}

func (s *fileService) buildPath(fileID uint, fileMap map[uint]*domain.File) string {
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
	return filepath.Join(parts...)
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

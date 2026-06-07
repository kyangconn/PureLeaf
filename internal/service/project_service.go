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
	Create(name string, ownerID uint) (*domain.Project, error)
	GetByID(projectID, userID uint) (*domain.Project, error)
	ListByUser(userID uint) ([]domain.Project, error)
	Update(projectID, userID uint, name string) (*domain.Project, error)
	Delete(projectID, userID uint) error
}

type projectService struct {
	projectRepo repository.ProjectRepository
	fileRepo    repository.FileRepository
	outputDir   string
}

// NewProjectService 创建项目服务
func NewProjectService(projectRepo repository.ProjectRepository, fileRepo repository.FileRepository, outputDir string) ProjectService {
	return &projectService{projectRepo: projectRepo, fileRepo: fileRepo, outputDir: outputDir}
}

func (s *projectService) Create(name string, ownerID uint) (*domain.Project, error) {
	project := &domain.Project{Name: name, OwnerID: ownerID}
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
	if err := os.WriteFile(filepath.Join(projDir, "main.tex"), []byte(defaultMainTex), 0644); err != nil {
		os.RemoveAll(projDir)
		s.fileRepo.Delete(mainTex.ID)
		s.projectRepo.Delete(project.ID)
		return nil, fmt.Errorf("写入模板文件失败: %w", err)
	}

	return project, nil
}

func (s *projectService) GetByID(projectID, userID uint) (*domain.Project, error) {
	project, err := s.projectRepo.FindByID(projectID)
	if err != nil {
		return nil, err
	}
	if project.OwnerID != userID {
		if ok, _ := s.projectRepo.IsCollaborator(projectID, userID); !ok {
			return nil, repository.ErrForbidden
		}
	}
	return project, nil
}

func (s *projectService) ListByUser(userID uint) ([]domain.Project, error) {
	own, err := s.projectRepo.FindByOwnerID(userID)
	if err != nil {
		return nil, err
	}
	collab, _ := s.projectRepo.FindByCollaborator(userID)
	return append(own, collab...), nil
}

func (s *projectService) Update(projectID, userID uint, name string) (*domain.Project, error) {
	project, err := s.GetByID(projectID, userID)
	if err != nil {
		return nil, err
	}
	if project.OwnerID != userID {
		return nil, repository.ErrForbidden
	}
	project.Name = name
	if err := s.projectRepo.Update(project); err != nil {
		return nil, err
	}
	return project, nil
}

func (s *projectService) Delete(projectID, userID uint) error {
	project, err := s.GetByID(projectID, userID)
	if err != nil {
		return err
	}
	if project.OwnerID != userID {
		return repository.ErrForbidden
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
}

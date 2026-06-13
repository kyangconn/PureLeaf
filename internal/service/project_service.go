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
	outputDir   string
}

// NewProjectService 创建项目服务
func NewProjectService(projectRepo repository.ProjectRepository, fileRepo repository.FileRepository, outputDir string) ProjectService {
	return &projectService{projectRepo: projectRepo, fileRepo: fileRepo, outputDir: outputDir}
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
	if err := os.WriteFile(filepath.Join(projDir, "main.tex"), []byte(defaultMainTex), 0644); err != nil {
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
	project, err := s.projectRepo.FindByID(projectID)
	if err != nil {
		return nil, err
	}
	project.Name = name
	if err := s.projectRepo.Update(project); err != nil {
		return nil, err
	}
	return project, nil
}

func (s *projectService) Delete(projectID uint) error {
	if _, err := s.projectRepo.FindByID(projectID); err != nil {
		return err
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

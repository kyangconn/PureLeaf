package service

import (
	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
	"gorm.io/gorm"
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
	db          *gorm.DB // 仅用于事务
}

// NewProjectService 创建项目服务
func NewProjectService(db *gorm.DB, projectRepo repository.ProjectRepository, fileRepo repository.FileRepository) ProjectService {
	return &projectService{db: db, projectRepo: projectRepo, fileRepo: fileRepo}
}

func (s *projectService) Create(name string, ownerID uint) (*domain.Project, error) {
	project := &domain.Project{Name: name, OwnerID: ownerID}

	err := s.db.Transaction(func(tx *gorm.DB) error {
		if err := s.projectRepo.Create(project); err != nil {
			return err
		}
		// 附带默认 main.tex
		mainTex := &domain.File{
			ProjectID: project.ID,
			Name:      "main.tex",
			Content:   defaultMainTex,
		}
		return s.fileRepo.Create(mainTex)
	})
	if err != nil {
		return nil, err
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
	return s.db.Transaction(func(tx *gorm.DB) error {
		if err := s.fileRepo.DeleteByProjectID(projectID); err != nil {
			return err
		}
		return s.projectRepo.Delete(projectID)
	})
}

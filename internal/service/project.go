package service

import (
	"errors"
	"fmt"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/model"
)

// ProjectService 项目管理业务逻辑
type ProjectService struct {
	db *gorm.DB
}

// NewProjectService 创建项目服务实例
func NewProjectService(db *gorm.DB) *ProjectService {
	return &ProjectService{db: db}
}

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

// Create 为用户创建一个新项目，并附带默认 main.tex 模板
func (s *ProjectService) Create(name string, ownerID uint) (*model.Project, error) {
	project := &model.Project{
		Name:    name,
		OwnerID: ownerID,
	}

	err := s.db.Transaction(func(tx *gorm.DB) error {
		if err := tx.Create(project).Error; err != nil {
			return fmt.Errorf("创建项目失败: %w", err)
		}
		// 附带默认 main.tex 模板
		mainTex := &model.File{
			ProjectID: project.ID,
			Name:      "main.tex",
			Content:   defaultMainTex,
		}
		if err := tx.Create(mainTex).Error; err != nil {
			return fmt.Errorf("创建默认文件失败: %w", err)
		}
		return nil
	})
	if err != nil {
		return nil, err
	}
	return project, nil
}

// GetByID 根据项目 ID 获取项目，并校验用户权限
func (s *ProjectService) GetByID(projectID, userID uint) (*model.Project, error) {
	var project model.Project
	if err := s.db.Preload("Owner").First(&project, projectID).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, errors.New("项目不存在")
		}
		return nil, fmt.Errorf("查询项目失败: %w", err)
	}

	// 权限校验: 仅拥有者或协作者可访问
	if project.OwnerID != userID && !s.isCollaborator(projectID, userID) {
		return nil, errors.New("无权限访问该项目")
	}

	return &project, nil
}

// ListByUser 获取用户参与的所有项目 (拥有的 + 协作的)
func (s *ProjectService) ListByUser(userID uint) ([]model.Project, error) {
	var projects []model.Project

	// 查询拥有的项目
	if err := s.db.Where("owner_id = ?", userID).Find(&projects).Error; err != nil {
		return nil, fmt.Errorf("查询项目失败: %w", err)
	}

	// 预留: 查询协作项目
	var collabIDs []uint
	s.db.Model(&model.Collaborator{}).
		Where("user_id = ?", userID).
		Pluck("project_id", &collabIDs)

	if len(collabIDs) > 0 {
		var collabProjects []model.Project
		s.db.Where("id IN ?", collabIDs).Find(&collabProjects)
		projects = append(projects, collabProjects...)
	}

	return projects, nil
}

// Update 更新项目名称 (仅拥有者可操作)
func (s *ProjectService) Update(projectID, userID uint, name string) (*model.Project, error) {
	project, err := s.GetByID(projectID, userID)
	if err != nil {
		return nil, err
	}
	if project.OwnerID != userID {
		return nil, errors.New("仅项目拥有者可修改")
	}

	project.Name = name
	if err := s.db.Save(project).Error; err != nil {
		return nil, fmt.Errorf("更新项目失败: %w", err)
	}
	return project, nil
}

// Delete 删除项目及其所有文件 (仅拥有者可操作)
func (s *ProjectService) Delete(projectID, userID uint) error {
	project, err := s.GetByID(projectID, userID)
	if err != nil {
		return err
	}
	if project.OwnerID != userID {
		return errors.New("仅项目拥有者可删除")
	}

	// 级联删除所有文件和协作记录
	return s.db.Transaction(func(tx *gorm.DB) error {
		if err := tx.Where("project_id = ?", projectID).Delete(&model.File{}).Error; err != nil {
			return err
		}
		if err := tx.Where("project_id = ?", projectID).Delete(&model.Collaborator{}).Error; err != nil {
			return err
		}
		if err := tx.Delete(&model.Project{}, projectID).Error; err != nil {
			return err
		}
		return nil
	})
}

// isCollaborator 检查用户是否为项目协作者 (预留)
func (s *ProjectService) isCollaborator(projectID, userID uint) bool {
	var count int64
	s.db.Model(&model.Collaborator{}).
		Where("project_id = ? AND user_id = ?", projectID, userID).
		Count(&count)
	return count > 0
}

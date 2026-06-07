package repository

import (
	"errors"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/domain"
)

// ---- ProjectRepository ----

// ProjectRepository 项目数据访问接口
type ProjectRepository interface {
	Create(project *domain.Project) error
	FindByID(id uint) (*domain.Project, error)
	FindByOwnerID(ownerID uint) ([]domain.Project, error)
	FindByCollaborator(userID uint) ([]domain.Project, error)
	Update(project *domain.Project) error
	Delete(id uint) error
	IsCollaborator(projectID, userID uint) (bool, error)
}

type projectRepo struct{ db *gorm.DB }

func NewProjectRepository(db *gorm.DB) ProjectRepository { return &projectRepo{db: db} }

func (r *projectRepo) Create(project *domain.Project) error {
	return r.db.Create(project).Error
}

func (r *projectRepo) FindByID(id uint) (*domain.Project, error) {
	var project domain.Project
	if err := r.db.Preload("Owner").First(&project, id).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrProjectNotFound
		}
		return nil, err
	}
	return &project, nil
}

func (r *projectRepo) FindByOwnerID(ownerID uint) ([]domain.Project, error) {
	var projects []domain.Project
	err := r.db.Where("owner_id = ?", ownerID).
		Order("updated_at DESC").Find(&projects).Error
	return projects, err
}

func (r *projectRepo) FindByCollaborator(userID uint) ([]domain.Project, error) {
	var ids []uint
	r.db.Model(&domain.Collaborator{}).
		Where("user_id = ?", userID).
		Pluck("project_id", &ids)
	if len(ids) == 0 {
		return nil, nil
	}
	var projects []domain.Project
	err := r.db.Where("id IN ?", ids).Find(&projects).Error
	return projects, err
}

func (r *projectRepo) Update(project *domain.Project) error {
	return r.db.Save(project).Error
}

func (r *projectRepo) Delete(id uint) error {
	return r.db.Delete(&domain.Project{}, id).Error
}

func (r *projectRepo) IsCollaborator(projectID, userID uint) (bool, error) {
	var count int64
	err := r.db.Model(&domain.Collaborator{}).
		Where("project_id = ? AND user_id = ?", projectID, userID).
		Count(&count).Error
	return count > 0, err
}

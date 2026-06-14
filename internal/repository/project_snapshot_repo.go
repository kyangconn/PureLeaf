package repository

import (
	"errors"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/domain"
)

// ProjectSnapshotRepository 项目快照数据访问接口
type ProjectSnapshotRepository interface {
	Create(snap *domain.ProjectSnapshot) error
	FindByID(id uint) (*domain.ProjectSnapshot, error)
	FindByProjectID(projectID uint, limit int) ([]domain.ProjectSnapshot, error)
	Delete(id uint) error
	DeleteByProjectID(projectID uint) error
}

type projectSnapshotRepo struct{ db *gorm.DB }

func NewProjectSnapshotRepository(db *gorm.DB) ProjectSnapshotRepository {
	return &projectSnapshotRepo{db: db}
}

func (r *projectSnapshotRepo) Create(snap *domain.ProjectSnapshot) error {
	return r.db.Create(snap).Error
}

func (r *projectSnapshotRepo) FindByID(id uint) (*domain.ProjectSnapshot, error) {
	var snap domain.ProjectSnapshot
	if err := r.db.First(&snap, id).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrSnapshotNotFound
		}
		return nil, err
	}
	return &snap, nil
}

func (r *projectSnapshotRepo) FindByProjectID(projectID uint, limit int) ([]domain.ProjectSnapshot, error) {
	var snaps []domain.ProjectSnapshot
	q := r.db.Where("project_id = ?", projectID).Order("created_at DESC, id DESC")
	if limit > 0 {
		q = q.Limit(limit)
	}
	err := q.Find(&snaps).Error
	return snaps, err
}

func (r *projectSnapshotRepo) Delete(id uint) error {
	return r.db.Delete(&domain.ProjectSnapshot{}, id).Error
}

func (r *projectSnapshotRepo) DeleteByProjectID(projectID uint) error {
	return r.db.Where("project_id = ?", projectID).Delete(&domain.ProjectSnapshot{}).Error
}

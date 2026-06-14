package repository

import (
	"errors"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/domain"
)

// FileRevisionRepository 文件历史版本数据访问接口
type FileRevisionRepository interface {
	Create(rev *domain.FileRevision) error
	FindByID(id uint) (*domain.FileRevision, error)
	FindByFileID(fileID uint, limit int) ([]domain.FileRevision, error)
	FindByProjectID(projectID uint, limit int) ([]domain.FileRevision, error)
	FindBySnapshotID(snapshotID uint) ([]domain.FileRevision, error)
	LatestByFileID(fileID uint) (*domain.FileRevision, error)
	DeleteByFileID(fileID uint) error
}

type fileRevisionRepo struct{ db *gorm.DB }

func NewFileRevisionRepository(db *gorm.DB) FileRevisionRepository {
	return &fileRevisionRepo{db: db}
}

func (r *fileRevisionRepo) Create(rev *domain.FileRevision) error {
	return r.db.Create(rev).Error
}

func (r *fileRevisionRepo) FindByID(id uint) (*domain.FileRevision, error) {
	var rev domain.FileRevision
	if err := r.db.First(&rev, id).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrRevisionNotFound
		}
		return nil, err
	}
	return &rev, nil
}

func (r *fileRevisionRepo) FindByFileID(fileID uint, limit int) ([]domain.FileRevision, error) {
	var revs []domain.FileRevision
	q := r.db.Where("file_id = ?", fileID).Order("created_at DESC, id DESC")
	if limit > 0 {
		q = q.Limit(limit)
	}
	err := q.Find(&revs).Error
	return revs, err
}

func (r *fileRevisionRepo) FindByProjectID(projectID uint, limit int) ([]domain.FileRevision, error) {
	var revs []domain.FileRevision
	q := r.db.Where("project_id = ?", projectID).Order("created_at DESC, id DESC")
	if limit > 0 {
		q = q.Limit(limit)
	}
	err := q.Find(&revs).Error
	return revs, err
}

func (r *fileRevisionRepo) FindBySnapshotID(snapshotID uint) ([]domain.FileRevision, error) {
	var revs []domain.FileRevision
	err := r.db.Where("snapshot_id = ?", snapshotID).
		Order("file_path ASC").Find(&revs).Error
	return revs, err
}

func (r *fileRevisionRepo) LatestByFileID(fileID uint) (*domain.FileRevision, error) {
	var rev domain.FileRevision
	err := r.db.Where("file_id = ?", fileID).
		Order("created_at DESC, id DESC").First(&rev).Error
	if err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrRevisionNotFound
		}
		return nil, err
	}
	return &rev, nil
}

func (r *fileRevisionRepo) DeleteByFileID(fileID uint) error {
	return r.db.Where("file_id = ?", fileID).Delete(&domain.FileRevision{}).Error
}

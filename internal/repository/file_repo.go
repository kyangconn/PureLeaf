package repository

import (
	"errors"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/domain"
)

// ---- FileRepository ----

// FileRepository 文件数据访问接口
type FileRepository interface {
	Create(file *domain.File) error
	FindByID(id uint) (*domain.File, error)
	FindByProjectID(projectID uint) ([]domain.File, error)
	FindByParentID(projectID, parentID uint) ([]domain.File, error)
	FindTeXFiles(projectID uint) ([]domain.File, error)
	Update(file *domain.File) error
	Delete(id uint) error
	DeleteByIDs(ids []uint) error
	DeleteByProjectID(projectID uint) error
}

type fileRepo struct{ db *gorm.DB }

func NewFileRepository(db *gorm.DB) FileRepository { return &fileRepo{db: db} }

func (r *fileRepo) Create(file *domain.File) error {
	return r.db.Create(file).Error
}

func (r *fileRepo) FindByID(id uint) (*domain.File, error) {
	var file domain.File
	if err := r.db.First(&file, id).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrFileNotFound
		}
		return nil, err
	}
	return &file, nil
}

func (r *fileRepo) FindByProjectID(projectID uint) ([]domain.File, error) {
	var files []domain.File
	err := r.db.Where("project_id = ?", projectID).
		Order("is_dir DESC, name ASC").Find(&files).Error
	return files, err
}

func (r *fileRepo) FindByParentID(projectID, parentID uint) ([]domain.File, error) {
	var files []domain.File
	err := r.db.Where("project_id = ? AND parent_id = ?", projectID, parentID).
		Find(&files).Error
	return files, err
}

func (r *fileRepo) FindTeXFiles(projectID uint) ([]domain.File, error) {
	var files []domain.File
	err := r.db.Where("project_id = ? AND is_dir = ?", projectID, false).Find(&files).Error
	return files, err
}

func (r *fileRepo) Update(file *domain.File) error {
	return r.db.Save(file).Error
}

func (r *fileRepo) Delete(id uint) error {
	return r.db.Delete(&domain.File{}, id).Error
}

func (r *fileRepo) DeleteByIDs(ids []uint) error {
	return r.db.Where("id IN ?", ids).Delete(&domain.File{}).Error
}

func (r *fileRepo) DeleteByProjectID(projectID uint) error {
	return r.db.Where("project_id = ?", projectID).Delete(&domain.File{}).Error
}

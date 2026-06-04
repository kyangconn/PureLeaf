// Package repository 定义数据访问层接口及 GORM 实现
package repository

import (
	"errors"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/domain"
)

// ---- Sentinel Errors ----

var (
	ErrUserNotFound    = errors.New("用户不存在")
	ErrUsernameExists  = errors.New("用户名已存在")
	ErrEmailExists     = errors.New("邮箱已存在")
	ErrProjectNotFound = errors.New("项目不存在")
	ErrFileNotFound    = errors.New("文件不存在")
	ErrForbidden       = errors.New("无权限")
)

// ---- UserRepository ----

// UserRepository 用户数据访问接口
type UserRepository interface {
	Create(user *domain.User) error
	FindByUsername(username string) (*domain.User, error)
	FindByID(id uint) (*domain.User, error)
	Count() (int64, error)
}

type userRepo struct{ db *gorm.DB }

func NewUserRepository(db *gorm.DB) UserRepository { return &userRepo{db: db} }

func (r *userRepo) Create(user *domain.User) error {
	if err := r.db.Create(user).Error; err != nil {
		if errors.Is(err, gorm.ErrDuplicatedKey) {
			return ErrUsernameExists
		}
		return err
	}
	return nil
}

func (r *userRepo) FindByUsername(username string) (*domain.User, error) {
	var user domain.User
	if err := r.db.Where("username = ?", username).First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrUserNotFound
		}
		return nil, err
	}
	return &user, nil
}

func (r *userRepo) FindByID(id uint) (*domain.User, error) {
	var user domain.User
	if err := r.db.First(&user, id).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, ErrUserNotFound
		}
		return nil, err
	}
	return &user, nil
}

func (r *userRepo) Count() (int64, error) {
	var count int64
	err := r.db.Model(&domain.User{}).Count(&count).Error
	return count, err
}

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

// Package service 提供业务逻辑层，依赖 repository 接口
package service

import (
	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
)

// UserService 认证业务接口
type UserService interface {
	CreateDefault() (*domain.User, error)
	GetByID(id uint) (*domain.User, error)
	HasUsers() (bool, error)
}

type userService struct {
	userRepo repository.UserRepository
}

// NewUserService 创建认证服务
func NewUserService(userRepo repository.UserRepository) UserService {
	return &userService{userRepo: userRepo}
}

func (s *userService) CreateDefault() (*domain.User, error) {
	user := &domain.User{
		Username: "admin",
		Email:    "admin@localhost",
	}
	if err := s.userRepo.Create(user); err != nil {
		return nil, err
	}
	return user, nil
}

func (s *userService) GetByID(id uint) (*domain.User, error) {
	return s.userRepo.FindByID(id)
}

func (s *userService) HasUsers() (bool, error) {
	count, err := s.userRepo.Count()
	return count > 0, err
}

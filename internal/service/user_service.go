// Package service 提供业务逻辑层，依赖 repository 接口
package service

import (
	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
	"github.com/kyangconn/goleaf/pkg/jwt"
	"github.com/kyangconn/goleaf/pkg/password"
)

// UserService 认证业务接口
type UserService interface {
	Register(username, email, pwd string) (*domain.User, string, error)
	Login(username, pwd string) (*domain.User, string, error)
	GetByID(id uint) (*domain.User, error)
	HasUsers() (bool, error)
}

type userService struct {
	userRepo   repository.UserRepository
	jwtManager *jwt.Manager
}

// NewUserService 创建认证服务
func NewUserService(userRepo repository.UserRepository, jwtMgr *jwt.Manager) UserService {
	return &userService{userRepo: userRepo, jwtManager: jwtMgr}
}

func (s *userService) Register(username, email, pwd string) (*domain.User, string, error) {
	hash, err := password.Hash(pwd)
	if err != nil {
		return nil, "", err
	}

	user := &domain.User{Username: username, Email: email, PasswordHash: hash}
	if err := s.userRepo.Create(user); err != nil {
		return nil, "", err
	}

	token, err := s.jwtManager.Generate(user.ID)
	if err != nil {
		return nil, "", err
	}

	return user, token, nil
}

func (s *userService) Login(username, pwd string) (*domain.User, string, error) {
	user, err := s.userRepo.FindByUsername(username)
	if err != nil {
		return nil, "", repository.ErrUserNotFound
	}

	if !password.Verify(pwd, user.PasswordHash) {
		return nil, "", repository.ErrUserNotFound
	}

	token, err := s.jwtManager.Generate(user.ID)
	if err != nil {
		return nil, "", err
	}

	return user, token, nil
}

func (s *userService) GetByID(id uint) (*domain.User, error) {
	return s.userRepo.FindByID(id)
}

func (s *userService) HasUsers() (bool, error) {
	count, err := s.userRepo.Count()
	return count > 0, err
}

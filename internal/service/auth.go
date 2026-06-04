// Package service 提供业务逻辑层
package service

import (
	"errors"
	"fmt"

	"golang.org/x/crypto/bcrypt"
	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/model"
)

// AuthService 用户认证相关业务逻辑
type AuthService struct {
	db *gorm.DB
}

// NewAuthService 创建认证服务实例
func NewAuthService(db *gorm.DB) *AuthService {
	return &AuthService{db: db}
}

// Register 注册新用户
func (s *AuthService) Register(username, email, password string) (*model.User, error) {
	// 密码哈希
	hash, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	if err != nil {
		return nil, fmt.Errorf("密码加密失败: %w", err)
	}

	user := &model.User{
		Username:     username,
		Email:        email,
		PasswordHash: string(hash),
	}

	if err := s.db.Create(user).Error; err != nil {
		if errors.Is(err, gorm.ErrDuplicatedKey) {
			return nil, errors.New("用户名或邮箱已存在")
		}
		return nil, fmt.Errorf("创建用户失败: %w", err)
	}

	return user, nil
}

// Login 用户登录，校验密码后返回用户信息
func (s *AuthService) Login(username, password string) (*model.User, error) {
	var user model.User
	if err := s.db.Where("username = ?", username).First(&user).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, errors.New("用户名或密码错误")
		}
		return nil, fmt.Errorf("查询用户失败: %w", err)
	}

	if err := bcrypt.CompareHashAndPassword([]byte(user.PasswordHash), []byte(password)); err != nil {
		return nil, errors.New("用户名或密码错误")
	}

	return &user, nil
}

// HasUsers 检查系统中是否已有用户 (用于首次启动判断)
func (s *AuthService) HasUsers() (bool, error) {
	var count int64
	if err := s.db.Model(&model.User{}).Count(&count).Error; err != nil {
		return false, fmt.Errorf("查询用户数量失败: %w", err)
	}
	return count > 0, nil
}

// GetByID 根据 ID 获取用户
func (s *AuthService) GetByID(id uint) (*model.User, error) {
	var user model.User
	if err := s.db.First(&user, id).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, errors.New("用户不存在")
		}
		return nil, fmt.Errorf("查询用户失败: %w", err)
	}
	return &user, nil
}

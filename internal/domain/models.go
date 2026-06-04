// Package domain 定义核心领域模型
package domain

import "time"

// User 用户
type User struct {
	ID           uint      `gorm:"primaryKey" json:"id"`
	Username     string    `gorm:"uniqueIndex;size:64;not null" json:"username"`
	Email        string    `gorm:"uniqueIndex;size:128;not null" json:"email"`
	PasswordHash string    `gorm:"size:256;not null" json:"-"`
	CreatedAt    time.Time `json:"created_at"`
	UpdatedAt    time.Time `json:"updated_at"`
}

func (User) TableName() string { return "users" }

// Project 项目
type Project struct {
	ID        uint      `gorm:"primaryKey" json:"id"`
	Name      string    `gorm:"size:255;not null" json:"name"`
	OwnerID   uint      `gorm:"index;not null" json:"owner_id"`
	Owner     *User     `gorm:"foreignKey:OwnerID" json:"owner,omitempty"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

func (Project) TableName() string { return "projects" }

// Collaborator 协作关系（预留）
type Collaborator struct {
	ID         uint      `gorm:"primaryKey" json:"id"`
	ProjectID  uint      `gorm:"uniqueIndex:idx_project_user;not null" json:"project_id"`
	UserID     uint      `gorm:"uniqueIndex:idx_project_user;not null" json:"user_id"`
	Permission string    `gorm:"size:16;default:write" json:"permission"`
	CreatedAt  time.Time `json:"created_at"`
}

func (Collaborator) TableName() string { return "collaborators" }

// File 文件/文件夹（邻接表）
type File struct {
	ID        uint      `gorm:"primaryKey" json:"id"`
	ProjectID uint      `gorm:"index;not null" json:"project_id"`
	ParentID  *uint     `gorm:"index" json:"parent_id"`
	Name      string    `gorm:"size:255;not null" json:"name"`
	IsDir     bool      `gorm:"default:false" json:"is_dir"`
	Content   string    `gorm:"type:text" json:"content,omitempty"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`

	Children []*File `gorm:"-" json:"children,omitempty"`
}

func (File) TableName() string { return "files" }

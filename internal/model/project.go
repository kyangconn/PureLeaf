package model

import (
	"time"
)

// Project 项目模型 — 对应一个 LaTeX 工程
type Project struct {
	ID        uint      `gorm:"primaryKey" json:"id"`
	Name      string    `gorm:"size:255;not null" json:"name"`
	OwnerID   uint      `gorm:"index;not null" json:"owner_id"`
	Owner     User      `gorm:"foreignKey:OwnerID" json:"owner,omitempty"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`
}

// TableName 指定表名
func (Project) TableName() string {
	return "projects"
}

// Collaborator 项目协作关系 — 预留多人协作能力
type Collaborator struct {
	ID         uint      `gorm:"primaryKey" json:"id"`
	ProjectID  uint      `gorm:"uniqueIndex:idx_project_user;not null" json:"project_id"`
	UserID     uint      `gorm:"uniqueIndex:idx_project_user;not null" json:"user_id"`
	Permission string    `gorm:"size:16;default:write" json:"permission"` // read | write
	CreatedAt  time.Time `json:"created_at"`
}

// TableName 指定表名
func (Collaborator) TableName() string {
	return "collaborators"
}

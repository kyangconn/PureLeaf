package model

import (
	"time"
)

// File 项目文件/文件夹 — 树形结构存储 (邻接表)
type File struct {
	ID        uint      `gorm:"primaryKey" json:"id"`
	ProjectID uint      `gorm:"index;not null" json:"project_id"`
	ParentID  *uint     `gorm:"index" json:"parent_id"` // NULL 表示根级节点
	Name      string    `gorm:"size:255;not null" json:"name"`
	IsDir     bool      `gorm:"default:false" json:"is_dir"`
	Content   string    `gorm:"type:text" json:"content,omitempty"` // 文件内容; 文件夹该字段为空
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`

	// 非数据库字段，仅用于 API 响应时构建树
	Children []*File `gorm:"-" json:"children,omitempty"`
}

// TableName 指定表名
func (File) TableName() string {
	return "files"
}

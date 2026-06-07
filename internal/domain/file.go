package domain

import "time"

// File 文件/文件夹（邻接表）
type File struct {
	ID        uint      `gorm:"primaryKey" json:"id"`
	ProjectID uint      `gorm:"index;not null" json:"project_id"`
	ParentID  *uint     `gorm:"index" json:"parent_id"`
	Name      string    `gorm:"size:255;not null" json:"name"`
	IsDir     bool      `gorm:"default:false" json:"is_dir"`
	Content   string    `gorm:"-" json:"content,omitempty"`
	CreatedAt time.Time `json:"created_at"`
	UpdatedAt time.Time `json:"updated_at"`

	Children []*File `gorm:"-" json:"children,omitempty"`
}

func (File) TableName() string { return "files" }

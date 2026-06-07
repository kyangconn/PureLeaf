package domain

import "time"

// Collaborator 协作关系（预留）
type Collaborator struct {
	ID         uint      `gorm:"primaryKey" json:"id"`
	ProjectID  uint      `gorm:"uniqueIndex:idx_project_user;not null" json:"project_id"`
	UserID     uint      `gorm:"uniqueIndex:idx_project_user;not null" json:"user_id"`
	Permission string    `gorm:"size:16;default:write" json:"permission"`
	CreatedAt  time.Time `json:"created_at"`
}

func (Collaborator) TableName() string { return "collaborators" }

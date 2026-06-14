package domain

import "time"

// ProjectSnapshot 记录项目在某次破坏性操作前的整体快照元数据。
//
// 破坏性操作（删除文件 / 删除项目 / 未来的 reset/checkout）执行前生成一条记录，
// 同时为项目内的每个文件写入一条 FileRevision 并关联到本快照（snapshot_id）。
// 内容本身存在 blob store，这里只保存引用与统计信息。
type ProjectSnapshot struct {
	ID         uint      `gorm:"primaryKey" json:"id"`
	ProjectID  uint      `gorm:"index;not null" json:"project_id"`
	Reason     string    `gorm:"size:32;not null;default:delete" json:"reason"`
	FileCount  int       `gorm:"not null;default:0" json:"file_count"`
	TotalSize  int64     `gorm:"not null;default:0" json:"total_size"`
	SnapshotOf string    `gorm:"size:512" json:"snapshot_of,omitempty"` // 快照针对的路径（如被删文件）
	CreatedAt  time.Time `gorm:"index" json:"created_at"`
}

func (ProjectSnapshot) TableName() string { return "project_snapshots" }

package domain

import "time"

// FileRevision 记录文件内容的某次历史版本。
//
// 设计要点：
//   - 内容采用 blob 寻址，相同内容共享同一个 blob（content_hash 去重）。
//   - file_id 在文件被删除后会变成悬空引用，这是预期行为：历史需保留。
//   - file_path 记录当时的相对路径，便于文件被改名/删除后仍可追溯。
//   - snapshot_id 非空表示该版本属于某次项目级快照（删除/reset 等破坏性操作前生成）。
type FileRevision struct {
	ID          uint      `gorm:"primaryKey" json:"id"`
	ProjectID   uint      `gorm:"index;not null" json:"project_id"`
	FileID      uint      `gorm:"index;not null" json:"file_id"`
	FilePath    string    `gorm:"size:512;not null" json:"file_path"`
	ContentHash string    `gorm:"size:64;index;not null" json:"content_hash"`
	BlobPath    string    `gorm:"size:512;not null" json:"blob_path"`
	Size        int64     `gorm:"not null;default:0" json:"size"`
	Reason      string    `gorm:"size:32;not null;default:save" json:"reason"`
	SnapshotID  *uint     `gorm:"index" json:"snapshot_id,omitempty"`
	CreatedAt   time.Time `gorm:"index" json:"created_at"`
}

func (FileRevision) TableName() string { return "file_revisions" }

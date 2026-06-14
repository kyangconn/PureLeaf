package service

import "errors"

// 文件版本（FileRevision.Reason）取值。
const (
	RevisionReasonSave   = "save"   // 普通保存
	RevisionReasonDelete = "delete" // 删除前快照
)

// 项目快照（ProjectSnapshot.Reason）取值。
const (
	SnapshotReasonDelete = "delete" // 删除文件/项目前
)

// 业务错误。
var (
	// ErrNameConflict 同级目录下存在同名文件/文件夹。
	ErrNameConflict = errors.New("同名文件已存在")
)

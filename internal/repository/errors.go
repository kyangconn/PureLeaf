// Package repository 定义数据访问层接口及 GORM 实现
package repository

import "errors"

// ---- Sentinel Errors ----

var (
	ErrProjectNotFound = errors.New("项目不存在")
	ErrFileNotFound    = errors.New("文件不存在")
)

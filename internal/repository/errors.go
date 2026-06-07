// Package repository 定义数据访问层接口及 GORM 实现
package repository

import "errors"

// ---- Sentinel Errors ----

var (
	ErrUserNotFound    = errors.New("用户不存在")
	ErrUsernameExists  = errors.New("用户名已存在")
	ErrEmailExists     = errors.New("邮箱已存在")
	ErrProjectNotFound = errors.New("项目不存在")
	ErrFileNotFound    = errors.New("文件不存在")
	ErrForbidden       = errors.New("无权限")
)

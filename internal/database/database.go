// Package database 提供数据库连接管理（仅 SQLite）
package database

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"
)

// NewSQLite 打开 SQLite 数据库连接（纯 Go 驱动，无需 CGO）
func NewSQLite(path string, debug bool) (*gorm.DB, error) {
	dir := filepath.Dir(path)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return nil, fmt.Errorf("创建数据库目录失败: %w", err)
	}

	logLevel := logger.Warn
	if debug {
		logLevel = logger.Info
	}

	db, err := gorm.Open(sqlite.Open(path), &gorm.Config{
		Logger: logger.Default.LogMode(logLevel),
	})
	if err != nil {
		return nil, fmt.Errorf("连接数据库失败: %w", err)
	}

	return db, nil
}

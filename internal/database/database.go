// Package database 负责数据库连接初始化与自动迁移
package database

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"

	"github.com/kyangconn/goleaf/internal/model"
)

// New 打开数据库连接并执行自动迁移
func New(path string, debug bool) (*gorm.DB, error) {
	// 确保数据库文件所在目录存在
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

	// 自动迁移表结构
	if err := db.AutoMigrate(
		&model.User{},
		&model.Project{},
		&model.Collaborator{},
		&model.File{},
	); err != nil {
		return nil, fmt.Errorf("数据库迁移失败: %w", err)
	}

	return db, nil
}

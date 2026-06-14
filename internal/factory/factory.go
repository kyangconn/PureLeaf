// Package factory 集中创建并管理所有应用依赖
package factory

import (
	"path/filepath"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/config"
	"github.com/kyangconn/goleaf/internal/database"
	"github.com/kyangconn/goleaf/internal/domain"
	pklog "github.com/kyangconn/goleaf/internal/log"
	"github.com/kyangconn/goleaf/internal/repository"
	"github.com/kyangconn/goleaf/internal/service"
)

// App 聚合所有应用依赖
type App struct {
	Config     *config.Config
	DB         *gorm.DB
	ProjectSvc service.ProjectService
	FileSvc    service.FileService
}

// New 初始化所有依赖并返回 App
func New() (*App, error) {
	cfg, err := config.Load("")
	if err != nil {
		return nil, err
	}
	pklog.Init(cfg.LogFile)

	db, err := database.NewSQLite(cfg.Database.Path, false)
	if err != nil {
		return nil, err
	}
	if err := db.AutoMigrate(&domain.Project{}, &domain.File{}); err != nil {
		return nil, err
	}

	projectRepo := repository.NewProjectRepository(db)
	fileRepo := repository.NewFileRepository(db)
	lockManager := service.NewProjectLockManager()

	projectSvc := service.NewProjectService(projectRepo, fileRepo, lockManager, filepath.Join(filepath.Dir(cfg.Database.Path), "projects"))
	fileSvc := service.NewFileService(fileRepo, projectRepo, lockManager, cfg.Latex.Compiler, cfg.Latex.Timeout, filepath.Join(filepath.Dir(cfg.Database.Path), "projects"))

	pklog.Infof("goleaf 已就绪")
	return &App{
		Config:     cfg,
		DB:         db,
		ProjectSvc: projectSvc,
		FileSvc:    fileSvc,
	}, nil
}

// Close releases resources owned by the application container.
func (a *App) Close() error {
	if a == nil || a.DB == nil {
		return nil
	}
	sqlDB, err := a.DB.DB()
	if err != nil {
		return err
	}
	return sqlDB.Close()
}

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
	Config      *config.Config
	DB          *gorm.DB
	ProjectSvc  service.ProjectService
	FileSvc     service.FileService
	LatexEnvSvc service.LatexEnvironmentService
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
	if err := db.AutoMigrate(
		&domain.Project{},
		&domain.File{},
		&domain.FileRevision{},
		&domain.ProjectSnapshot{},
	); err != nil {
		return nil, err
	}

	// 数据根目录 = goleaf.db 所在目录；项目、备份、blob 都挂在它下面。
	dataRoot := filepath.Dir(cfg.Database.Path)
	projectsDir := filepath.Join(dataRoot, "projects")
	backupDir := filepath.Join(dataRoot, ".backup")
	blobsDir := filepath.Join(backupDir, "blobs")

	projectRepo := repository.NewProjectRepository(db)
	fileRepo := repository.NewFileRepository(db)
	revRepo := repository.NewFileRevisionRepository(db)
	snapRepo := repository.NewProjectSnapshotRepository(db)
	lockManager := service.NewProjectLockManager()
	blobStore, err := service.NewBlobStore(blobsDir)
	if err != nil {
		return nil, err
	}

	projectSvc := service.NewProjectService(projectRepo, fileRepo, revRepo, snapRepo, lockManager, blobStore, projectsDir)
	fileSvc := service.NewFileService(fileRepo, projectRepo, revRepo, snapRepo, lockManager, blobStore, cfg.Latex.Compiler, cfg.Latex.Timeout, projectsDir)
	latexEnvSvc := service.NewLatexEnvironmentService(cfg.Latex.Compiler, filepath.Join(dataRoot, "downloads", "texlive"))

	pklog.Infof("goleaf 已就绪")
	return &App{
		Config:      cfg,
		DB:          db,
		ProjectSvc:  projectSvc,
		FileSvc:     fileSvc,
		LatexEnvSvc: latexEnvSvc,
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

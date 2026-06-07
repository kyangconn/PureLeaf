// goleaf — 轻量级在线 LaTeX 编辑器
package main

import (
	"context"
	"embed"
	"errors"
	"flag"
	"fmt"
	"io/fs"
	"net/http"
	"os"
	"os/signal"
	"path/filepath"
	"strings"
	"syscall"
	"time"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/config"
	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/handler"
	"github.com/kyangconn/goleaf/internal/repository"
	"github.com/kyangconn/goleaf/internal/router"
	"github.com/kyangconn/goleaf/internal/service"
	"github.com/kyangconn/goleaf/pkg/database"
	jwtpkg "github.com/kyangconn/goleaf/pkg/jwt"
	pklog "github.com/kyangconn/goleaf/pkg/log"
)

//go:embed dist/*
var webDist embed.FS

// ---- 依赖容器 ----

type dependencies struct {
	User    *handler.UserHandler
	Project *handler.ProjectHandler
	File    *handler.FileHandler
	JWT     *jwtpkg.Manager
	DB      *gorm.DB
}

func main() {
	parseFlags()

	cfg, err := config.Load("")
	if err != nil {
		pklog.Fatalf("加载配置失败: %v", err)
	}
	pklog.Init(cfg.Server.LogFile)
	pklog.Infof("goleaf v0.1.0")
	pklog.Infof("port=%d mode=%s jwt_expire=%dh", cfg.Server.Port, cfg.Server.Mode, cfg.JWT.ExpireHour)

	gin.SetMode(cfg.Server.Mode)

	db, err := database.NewSQLite(cfg.Database.Path, cfg.Server.Mode == "debug")
	if err != nil {
		pklog.Fatalf("数据库初始化失败: %v", err)
	}
	if err := db.AutoMigrate(&domain.User{}, &domain.Project{}, &domain.Collaborator{}, &domain.File{}); err != nil {
		pklog.Fatalf("数据库迁移失败: %v", err)
	}
	pklog.Infof("数据库已就绪")
	defer func() {
		if sqlDB, _ := db.DB(); sqlDB != nil {
			sqlDB.Close()
		}
	}()

	deps := initDependencies(db, cfg)
	r := router.Setup(deps.User, deps.Project, deps.File, deps.JWT, cfg)
	configureStaticAssets(r)
	startServer(r, cfg)
}

// ---- 命令行参数 ----

func parseFlags() {
	configFile := flag.String("config", "", "配置文件路径")
	flag.Parse()
	if *configFile != "" {
		os.Setenv("GOLEAF_CONFIG_FILE", *configFile)
	}
}

// ---- 依赖注入 ----

func initDependencies(db *gorm.DB, cfg *config.Config) *dependencies {
	jwtMgr := jwtpkg.NewManager(cfg.JWT.Secret, cfg.JWT.ExpireHour)

	userRepo := repository.NewUserRepository(db)
	projectRepo := repository.NewProjectRepository(db)
	fileRepo := repository.NewFileRepository(db)

	userSvc := service.NewUserService(userRepo, jwtMgr)
	dataDir := filepath.Join(filepath.Dir(cfg.Database.Path), "projects")
	projectSvc := service.NewProjectService(projectRepo, fileRepo, dataDir)
	fileSvc := service.NewFileService(fileRepo, projectRepo, cfg.Latex.Compiler, cfg.Latex.Timeout, dataDir)

	return &dependencies{
		User:    handler.NewUserHandler(userSvc),
		Project: handler.NewProjectHandler(projectSvc),
		File:    handler.NewFileHandler(fileSvc),
		JWT:     jwtMgr,
		DB:      db,
	}
}

// ---- 静态资源 (嵌入式 fs.FS) ----

func configureStaticAssets(r *gin.Engine) {
	distFS, err := fs.Sub(webDist, "dist")
	if err != nil {
		return
	}

	r.NoRoute(func(c *gin.Context) {
		if strings.HasPrefix(c.Request.URL.Path, "/api") {
			c.JSON(http.StatusNotFound, gin.H{"error": "接口不存在"})
			return
		}

		path := strings.TrimPrefix(c.Request.URL.Path, "/")
		if path == "" {
			path = "index.html"
		}

		data, ct, err := readEmbedFile(distFS, path)
		if err != nil {
			// SPA fallback
			data, ct, err = readEmbedFile(distFS, "index.html")
			if err != nil {
				c.String(http.StatusNotFound, "Not Found")
				return
			}
		}
		c.Data(http.StatusOK, ct, data)
	})
}

func readEmbedFile(fsys fs.FS, name string) ([]byte, string, error) {
	f, err := fsys.Open(name)
	if err != nil {
		return nil, "", err
	}
	defer f.Close()

	if stat, err := f.Stat(); err != nil || stat.IsDir() {
		return nil, "", fmt.Errorf("not a file")
	}

	data, err := fs.ReadFile(fsys, name)
	return data, mimeByExt(name), err
}

func mimeByExt(name string) string {
	switch {
	case strings.HasSuffix(name, ".html"):
		return "text/html; charset=utf-8"
	case strings.HasSuffix(name, ".css"):
		return "text/css; charset=utf-8"
	case strings.HasSuffix(name, ".js"):
		return "application/javascript; charset=utf-8"
	case strings.HasSuffix(name, ".json"):
		return "application/json; charset=utf-8"
	case strings.HasSuffix(name, ".png"):
		return "image/png"
	case strings.HasSuffix(name, ".jpg"), strings.HasSuffix(name, ".jpeg"):
		return "image/jpeg"
	case strings.HasSuffix(name, ".svg"):
		return "image/svg+xml"
	case strings.HasSuffix(name, ".ico"):
		return "image/x-icon"
	case strings.HasSuffix(name, ".woff"):
		return "font/woff"
	case strings.HasSuffix(name, ".woff2"):
		return "font/woff2"
	default:
		return "application/octet-stream"
	}
}

// ---- 服务器启动 ----

func startServer(r *gin.Engine, cfg *config.Config) {
	port := cfg.Server.Port
	if port == 0 {
		port = 8080
	}

	srv := &http.Server{
		Addr:    fmt.Sprintf(":%d", port),
		Handler: r,
	}

	go func() {
		pklog.Infof("服务已启动 http://localhost:%d", port)
		if err := srv.ListenAndServe(); err != nil && !errors.Is(err, http.ErrServerClosed) {
			if strings.Contains(err.Error(), "address already in use") {
				pklog.Fatalf("端口 %d 已被占用", port)
			}
			pklog.Fatalf("服务启动失败: %v", err)
		}
	}()

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit
	pklog.Infof("正在退出...")

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	if err := srv.Shutdown(ctx); err != nil {
		pklog.Fatalf("强制退出: %v", err)
	}
	pklog.Infof("已退出")
}

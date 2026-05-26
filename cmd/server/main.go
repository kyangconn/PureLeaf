// goleaf — 轻量级在线 LaTeX 编辑器
// 入口文件: 初始化配置、数据库、路由，启动 HTTP 服务
package main

import (
	"flag"
	"fmt"
	"log"
	"os"
	"path/filepath"

	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/config"
	"github.com/kyangconn/goleaf/internal/database"
	"github.com/kyangconn/goleaf/internal/handler"
	"github.com/kyangconn/goleaf/internal/middleware"
	"github.com/kyangconn/goleaf/internal/service"
)

func main() {
	// --- 命令行参数 ---
	configPath := flag.String("config", "", "配置文件路径 (默认自动搜索 config.yaml)")
	flag.Parse()

	// --- 加载配置 ---
	cfg, err := config.Load(*configPath)
	if err != nil {
		log.Fatalf("加载配置失败: %v", err)
	}

	// --- 设置 Gin 运行模式 ---
	gin.SetMode(cfg.Server.Mode)

	// --- 初始化数据库 ---
	db, err := database.New(cfg.Database.Path, cfg.Server.Mode == "debug")
	if err != nil {
		log.Fatalf("初始化数据库失败: %v", err)
	}
	log.Println("数据库初始化完成")

	// --- 初始化服务层 ---
	authSvc := service.NewAuthService(db)
	projectSvc := service.NewProjectService(db)

	// 编译输出目录
	outputDir := filepath.Join(filepath.Dir(cfg.Database.Path), "output")
	fileSvc := service.NewFileService(db, cfg.Latex.Compiler, cfg.Latex.Timeout, outputDir)

	// --- 初始化 HTTP 处理器 ---
	authH := handler.NewAuthHandler(authSvc, cfg.JWT.Secret, cfg.JWT.ExpireHour)
	projectH := handler.NewProjectHandler(projectSvc)
	fileH := handler.NewFileHandler(fileSvc)

	// --- 路由设置 ---
	r := gin.New()
	r.Use(gin.Logger(), gin.Recovery())
	r.Use(middleware.CORS()) // 允许前端跨域

	// 静态文件服务 — 生产模式下提供前端构建产物
	distPath := "./web/dist"
	if _, err := os.Stat(distPath); err == nil {
		r.Static("/assets", filepath.Join(distPath, "assets"))
		r.StaticFile("/favicon.ico", filepath.Join(distPath, "favicon.ico"))
		r.NoRoute(func(c *gin.Context) {
			// SPA 回退: 所有非 API 路由返回 index.html
			if !isAPIPath(c.Request.URL.Path) {
				c.File(filepath.Join(distPath, "index.html"))
			} else {
				c.JSON(404, gin.H{"error": "接口不存在"})
			}
		})
		log.Println("前端静态文件服务已启用 (web/dist)")
	}

	// --- API 路由 ---
	api := r.Group("/api")
	{
		// 公开接口 (无需认证)
		auth := api.Group("/auth")
		{
			auth.POST("/register", authH.Register)
			auth.POST("/login", authH.Login)
		}

		// 需认证接口
		authorized := api.Group("")
		authorized.Use(middleware.JWTAuth(cfg.JWT.Secret))
		{
			authorized.GET("/auth/me", authH.Me)

			// 项目管理
			projects := authorized.Group("/projects")
			{
				projects.GET("", projectH.List)
				projects.POST("", projectH.Create)
				projects.GET("/:id", projectH.Get)
				projects.PUT("/:id", projectH.Update)
				projects.DELETE("/:id", projectH.Delete)

				// 项目文件管理
				projects.GET("/:id/files", fileH.GetTree)
				projects.POST("/:id/files", fileH.Create)
				projects.GET("/:id/files/:fileId", fileH.GetContent)
				projects.PUT("/:id/files/:fileId", fileH.UpdateContent)
				projects.PATCH("/:id/files/:fileId/rename", fileH.Rename)
				projects.DELETE("/:id/files/:fileId", fileH.Delete)

				// LaTeX 编译
				projects.POST("/:id/compile", fileH.Compile)
			}
		}
	}

	// --- 启动服务 ---
	addr := fmt.Sprintf(":%d", cfg.Server.Port)
	log.Printf("goleaf 启动于 http://localhost%s", addr)
	if err := r.Run(addr); err != nil {
		log.Fatalf("服务启动失败: %v", err)
	}
}

// isAPIPath 判断请求路径是否为 API 路径
func isAPIPath(path string) bool {
	return len(path) >= 4 && path[:4] == "/api"
}

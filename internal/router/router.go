// Package router 集中管理 API 路由注册
package router

import (
	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/config"
	"github.com/kyangconn/goleaf/internal/handler"
	"github.com/kyangconn/goleaf/internal/middleware"
	jwtpkg "github.com/kyangconn/goleaf/pkg/jwt"
)

// Setup 注册所有 API 路由，返回 *gin.Engine
func Setup(
	userH *handler.UserHandler,
	projectH *handler.ProjectHandler,
	fileH *handler.FileHandler,
	jwtMgr *jwtpkg.Manager,
	cfg *config.Config,
) *gin.Engine {
	r := gin.New()
	r.Use(gin.Logger(), gin.Recovery())
	r.Use(middleware.CORS())

	api := r.Group("/api")
	{
		auth := api.Group("/auth")
		{
			auth.GET("/status", userH.Status)
			auth.POST("/login", userH.Login)

			// setup（注册）接口仅允许受信任 IP 访问
			setup := auth.Group("")
			setup.Use(middleware.SetupIPFilter(cfg.Server.TrustedProxies))
			setup.POST("/register", userH.Register)
		}

		authorized := api.Group("")
		authorized.Use(middleware.JWTAuth(jwtMgr))
		{
			authorized.GET("/auth/me", userH.Me)

			projects := authorized.Group("/projects")
			{
				projects.GET("", projectH.List)
				projects.POST("", projectH.Create)
				projects.GET("/:id", projectH.Get)
				projects.PUT("/:id", projectH.Update)
				projects.DELETE("/:id", projectH.Delete)

				projects.GET("/:id/files", fileH.GetTree)
				projects.POST("/:id/files", fileH.Create)
				projects.GET("/:id/files/:fileId", fileH.GetContent)
				projects.PUT("/:id/files/:fileId", fileH.UpdateContent)
				projects.PATCH("/:id/files/:fileId/rename", fileH.Rename)
				projects.DELETE("/:id/files/:fileId", fileH.Delete)

				projects.POST("/:id/compile", fileH.Compile)
			}
		}
	}
	return r
}

package handler

import (
	"errors"

	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
	"github.com/kyangconn/goleaf/internal/service"
)

// AuthHandler 认证相关 HTTP 处理器
type AuthHandler struct {
	svc service.AuthService
}

// NewAuthHandler 创建认证处理器
func NewAuthHandler(svc service.AuthService) *AuthHandler {
	return &AuthHandler{svc: svc}
}

// ---- 请求/响应结构体 ----

type registerReq struct {
	Username string `json:"username" binding:"required,min=3,max=64"`
	Email    string `json:"email" binding:"required,email"`
	Password string `json:"password" binding:"required,min=6"`
}

type loginReq struct {
	Username string `json:"username" binding:"required"`
	Password string `json:"password" binding:"required"`
}

// userVO 用户视图对象（不含密码哈希）
type userVO struct {
	ID       uint   `json:"id"`
	Username string `json:"username"`
	Email    string `json:"email"`
}

// Register 用户注册
// POST /api/auth/register
func (h *AuthHandler) Register(c *gin.Context) {
	var req registerReq
	if err := c.ShouldBindJSON(&req); err != nil {
		BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	user, token, err := h.svc.Register(req.Username, req.Email, req.Password)
	if err != nil {
		if errors.Is(err, repository.ErrUsernameExists) || errors.Is(err, repository.ErrEmailExists) {
			Error(c, 409, err)
			return
		}
		Error(c, 500, err)
		return
	}

	Created(c, gin.H{
		"token": token,
		"user":  toUserVO(user),
	})
}

// Login 用户登录
// POST /api/auth/login
func (h *AuthHandler) Login(c *gin.Context) {
	var req loginReq
	if err := c.ShouldBindJSON(&req); err != nil {
		BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	user, token, err := h.svc.Login(req.Username, req.Password)
	if err != nil {
		if errors.Is(err, repository.ErrUserNotFound) {
			Unauthorized(c, "用户名或密码错误")
			return
		}
		Error(c, 500, err)
		return
	}

	Success(c, gin.H{
		"token": token,
		"user":  toUserVO(user),
	})
}

// Status 返回系统初始化状态（是否有用户）
// GET /api/auth/status
func (h *AuthHandler) Status(c *gin.Context) {
	hasUsers, err := h.svc.HasUsers()
	if err != nil {
		Error(c, 500, err)
		return
	}
	Success(c, gin.H{"has_users": hasUsers})
}

// Me 获取当前登录用户信息
// GET /api/auth/me
func (h *AuthHandler) Me(c *gin.Context) {
	userID := c.GetUint("userID")

	user, err := h.svc.GetByID(userID)
	if err != nil {
		if errors.Is(err, repository.ErrUserNotFound) {
			NotFound(c, "用户不存在")
			return
		}
		Error(c, 500, err)
		return
	}

	Success(c, toUserVO(user))
}

func toUserVO(u *domain.User) userVO {
	return userVO{
		ID:       u.ID,
		Username: u.Username,
		Email:    u.Email,
	}
}

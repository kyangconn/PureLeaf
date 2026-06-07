package handler

import (
	"errors"

	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
	"github.com/kyangconn/goleaf/internal/service"
	resp "github.com/kyangconn/goleaf/pkg/response"
)

// UserHandler 用户相关 HTTP 处理器
type UserHandler struct {
	svc service.UserService
}

// NewUserHandler 创建用户处理器
func NewUserHandler(svc service.UserService) *UserHandler {
	return &UserHandler{svc: svc}
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
func (h *UserHandler) Register(c *gin.Context) {
	var req registerReq
	if err := c.ShouldBindJSON(&req); err != nil {
		resp.BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	user, token, err := h.svc.Register(req.Username, req.Email, req.Password)
	if err != nil {
		if errors.Is(err, repository.ErrUsernameExists) || errors.Is(err, repository.ErrEmailExists) {
			resp.Fail(c, 409, err)
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.Created(c, gin.H{
		"token": token,
		"user":  toUserVO(user),
	})
}

// Login 用户登录
// POST /api/auth/login
func (h *UserHandler) Login(c *gin.Context) {
	var req loginReq
	if err := c.ShouldBindJSON(&req); err != nil {
		resp.BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	user, token, err := h.svc.Login(req.Username, req.Password)
	if err != nil {
		if errors.Is(err, repository.ErrUserNotFound) {
			resp.Unauthorized(c, "用户名或密码错误")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, gin.H{
		"token": token,
		"user":  toUserVO(user),
	})
}

// Status 返回系统初始化状态（是否有用户）
// GET /api/auth/status
func (h *UserHandler) Status(c *gin.Context) {
	hasUsers, err := h.svc.HasUsers()
	if err != nil {
		resp.Fail(c, 500, err)
		return
	}
	resp.OK(c, gin.H{"has_users": hasUsers})
}

// Me 获取当前登录用户信息
// GET /api/auth/me
func (h *UserHandler) Me(c *gin.Context) {
	userID := c.GetUint("userID")

	user, err := h.svc.GetByID(userID)
	if err != nil {
		if errors.Is(err, repository.ErrUserNotFound) {
			resp.NotFound(c, "用户不存在")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, toUserVO(user))
}

func toUserVO(u *domain.User) userVO {
	return userVO{
		ID:       u.ID,
		Username: u.Username,
		Email:    u.Email,
	}
}

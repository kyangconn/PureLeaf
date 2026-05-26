package handler

import (
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/golang-jwt/jwt/v5"

	"github.com/kyangconn/goleaf/internal/model"
	"github.com/kyangconn/goleaf/internal/service"
)

// AuthHandler 认证相关 HTTP 处理器
type AuthHandler struct {
	svc        *service.AuthService
	jwtSecret  string
	jwtExpHour int
}

// NewAuthHandler 创建认证处理器
func NewAuthHandler(svc *service.AuthService, jwtSecret string, jwtExpHour int) *AuthHandler {
	return &AuthHandler{svc: svc, jwtSecret: jwtSecret, jwtExpHour: jwtExpHour}
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

type authResp struct {
	Token string `json:"token"`
	User  userVO `json:"user"`
}

type userVO struct {
	ID       uint   `json:"id"`
	Username string `json:"username"`
	Email    string `json:"email"`
}

// Register 用户注册
func (h *AuthHandler) Register(c *gin.Context) {
	var req registerReq
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "请求参数无效: " + err.Error()})
		return
	}

	user, err := h.svc.Register(req.Username, req.Email, req.Password)
	if err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": err.Error()})
		return
	}

	token, err := h.generateToken(user.ID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "生成令牌失败"})
		return
	}

	c.JSON(http.StatusCreated, authResp{
		Token: token,
		User:  toUserVO(user),
	})
}

// Login 用户登录
func (h *AuthHandler) Login(c *gin.Context) {
	var req loginReq
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "请求参数无效: " + err.Error()})
		return
	}

	user, err := h.svc.Login(req.Username, req.Password)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": err.Error()})
		return
	}

	token, err := h.generateToken(user.ID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "生成令牌失败"})
		return
	}

	c.JSON(http.StatusOK, authResp{
		Token: token,
		User:  toUserVO(user),
	})
}

// Me 获取当前登录用户信息
func (h *AuthHandler) Me(c *gin.Context) {
	userID := c.GetUint("userID")

	user, err := h.svc.GetByID(userID)
	if err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, toUserVO(user))
}

// generateToken 生成 JWT Token
func (h *AuthHandler) generateToken(userID uint) (string, error) {
	claims := jwt.MapClaims{
		"user_id": userID,
		"exp":     time.Now().Add(time.Duration(h.jwtExpHour) * time.Hour).Unix(),
		"iat":     time.Now().Unix(),
	}
	token := jwt.NewWithClaims(jwt.SigningMethodHS256, claims)
	return token.SignedString([]byte(h.jwtSecret))
}

func toUserVO(u *model.User) userVO {
	return userVO{
		ID:       u.ID,
		Username: u.Username,
		Email:    u.Email,
	}
}

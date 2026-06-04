// Package handler 统一 HTTP 响应格式
package handler

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// Response 统一 API 响应结构
type Response struct {
	Code    int         `json:"code"`
	Message string      `json:"message"`
	Data    interface{} `json:"data,omitempty"`
	Error   string      `json:"error,omitempty"`
}

// Success 200 OK 成功响应
func Success(c *gin.Context, data interface{}) {
	c.JSON(http.StatusOK, Response{
		Code:    http.StatusOK,
		Message: "success",
		Data:    data,
	})
}

// Created 201 Created 创建成功响应
func Created(c *gin.Context, data interface{}) {
	c.JSON(http.StatusCreated, Response{
		Code:    http.StatusCreated,
		Message: "created",
		Data:    data,
	})
}

// Error 通用错误响应
func Error(c *gin.Context, httpStatus int, err error) {
	c.AbortWithStatusJSON(httpStatus, Response{
		Code:    httpStatus,
		Message: http.StatusText(httpStatus),
		Error:   err.Error(),
	})
}

// BadRequest 400 参数错误
func BadRequest(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusBadRequest, Response{
		Code:    http.StatusBadRequest,
		Message: "bad request",
		Error:   msg,
	})
}

// Unauthorized 401 未认证
func Unauthorized(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusUnauthorized, Response{
		Code:    http.StatusUnauthorized,
		Message: "unauthorized",
		Error:   msg,
	})
}

// Forbidden 403 无权限
func Forbidden(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusForbidden, Response{
		Code:    http.StatusForbidden,
		Message: "forbidden",
		Error:   msg,
	})
}

// NotFound 404 资源不存在
func NotFound(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusNotFound, Response{
		Code:    http.StatusNotFound,
		Message: "not found",
		Error:   msg,
	})
}

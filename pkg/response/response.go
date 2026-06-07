// Package response 提供统一的 HTTP JSON 响应格式与便捷构造函数。
// handler 层和 middleware 层均使用此包返回一致的数据结构。
package response

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// Body 统一的 API 响应体
type Body struct {
	Code    int    `json:"code"`
	Message string `json:"message"`
	Data    any    `json:"data,omitempty"`
	Error   string `json:"error,omitempty"`
}

// OK 200 — 成功响应
func OK(c *gin.Context, data any) {
	c.JSON(http.StatusOK, Body{
		Code:    http.StatusOK,
		Message: "success",
		Data:    data,
	})
}

// Created 201 — 创建成功
func Created(c *gin.Context, data any) {
	c.JSON(http.StatusCreated, Body{
		Code:    http.StatusCreated,
		Message: "created",
		Data:    data,
	})
}

// Fail 通用错误（带 HTTP 状态码）
func Fail(c *gin.Context, httpStatus int, err error) {
	c.AbortWithStatusJSON(httpStatus, Body{
		Code:    httpStatus,
		Message: http.StatusText(httpStatus),
		Error:   err.Error(),
	})
}

// BadRequest 400
func BadRequest(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusBadRequest, Body{
		Code:    http.StatusBadRequest,
		Message: "bad request",
		Error:   msg,
	})
}

// Unauthorized 401
func Unauthorized(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusUnauthorized, Body{
		Code:    http.StatusUnauthorized,
		Message: "unauthorized",
		Error:   msg,
	})
}

// Forbidden 403
func Forbidden(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusForbidden, Body{
		Code:    http.StatusForbidden,
		Message: "forbidden",
		Error:   msg,
	})
}

// NotFound 404
func NotFound(c *gin.Context, msg string) {
	c.AbortWithStatusJSON(http.StatusNotFound, Body{
		Code:    http.StatusNotFound,
		Message: "not found",
		Error:   msg,
	})
}

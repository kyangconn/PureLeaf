package handler

import (
	"net/http"
	"strconv"

	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/model"
	"github.com/kyangconn/goleaf/internal/service"
)

// ProjectHandler 项目管理 HTTP 处理器
type ProjectHandler struct {
	svc *service.ProjectService
}

// NewProjectHandler 创建项目管理处理器
func NewProjectHandler(svc *service.ProjectService) *ProjectHandler {
	return &ProjectHandler{svc: svc}
}

// ---- 请求结构体 ----

type createProjectReq struct {
	Name string `json:"name" binding:"required,min=1,max=255"`
}

type updateProjectReq struct {
	Name string `json:"name" binding:"required,min=1,max=255"`
}

// List 获取用户的项目列表
func (h *ProjectHandler) List(c *gin.Context) {
	userID := c.GetUint("userID")

	projects, err := h.svc.ListByUser(userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}

	// 确保返回空数组而非 null
	if projects == nil {
		projects = []model.Project{}
	}

	c.JSON(http.StatusOK, projects)
}

// Get 获取单个项目详情
func (h *ProjectHandler) Get(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的项目 ID"})
		return
	}

	project, err := h.svc.GetByID(uint(projectID), userID)
	if err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, project)
}

// Create 创建新项目
func (h *ProjectHandler) Create(c *gin.Context) {
	userID := c.GetUint("userID")

	var req createProjectReq
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "请求参数无效: " + err.Error()})
		return
	}

	project, err := h.svc.Create(req.Name, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusCreated, project)
}

// Update 更新项目名称
func (h *ProjectHandler) Update(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的项目 ID"})
		return
	}

	var req updateProjectReq
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "请求参数无效: " + err.Error()})
		return
	}

	project, err := h.svc.Update(uint(projectID), userID, req.Name)
	if err != nil {
		c.JSON(http.StatusForbidden, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, project)
}

// Delete 删除项目
func (h *ProjectHandler) Delete(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的项目 ID"})
		return
	}

	if err := h.svc.Delete(uint(projectID), userID); err != nil {
		c.JSON(http.StatusForbidden, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "项目已删除"})
}

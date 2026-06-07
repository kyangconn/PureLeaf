package handler

import (
	"errors"
	"strconv"

	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/repository"
	"github.com/kyangconn/goleaf/internal/service"
	resp "github.com/kyangconn/goleaf/pkg/response"
)

// ProjectHandler 项目管理 HTTP 处理器
type ProjectHandler struct {
	svc service.ProjectService
}

// NewProjectHandler 创建项目管理处理器
func NewProjectHandler(svc service.ProjectService) *ProjectHandler {
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
// GET /api/projects
func (h *ProjectHandler) List(c *gin.Context) {
	userID := c.GetUint("userID")

	projects, err := h.svc.ListByUser(userID)
	if err != nil {
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, projects)
}

// Get 获取单个项目详情
// GET /api/projects/:id
func (h *ProjectHandler) Get(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		resp.BadRequest(c, "无效的项目 ID")
		return
	}

	project, err := h.svc.GetByID(uint(projectID), userID)
	if err != nil {
		if errors.Is(err, repository.ErrProjectNotFound) {
			resp.NotFound(c, "项目不存在")
			return
		}
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权访问该项目")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, project)
}

// Create 创建新项目
// POST /api/projects
func (h *ProjectHandler) Create(c *gin.Context) {
	userID := c.GetUint("userID")

	var req createProjectReq
	if err := c.ShouldBindJSON(&req); err != nil {
		resp.BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	project, err := h.svc.Create(req.Name, userID)
	if err != nil {
		resp.Fail(c, 500, err)
		return
	}

	resp.Created(c, project)
}

// Update 更新项目名称
// PUT /api/projects/:id
func (h *ProjectHandler) Update(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		resp.BadRequest(c, "无效的项目 ID")
		return
	}

	var req updateProjectReq
	if err := c.ShouldBindJSON(&req); err != nil {
		resp.BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	project, err := h.svc.Update(uint(projectID), userID, req.Name)
	if err != nil {
		if errors.Is(err, repository.ErrProjectNotFound) {
			resp.NotFound(c, "项目不存在")
			return
		}
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "仅项目所有者可编辑")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, project)
}

// Delete 删除项目
// DELETE /api/projects/:id
func (h *ProjectHandler) Delete(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		resp.BadRequest(c, "无效的项目 ID")
		return
	}

	if err := h.svc.Delete(uint(projectID), userID); err != nil {
		if errors.Is(err, repository.ErrProjectNotFound) {
			resp.NotFound(c, "项目不存在")
			return
		}
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "仅项目所有者可删除")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, gin.H{"message": "项目已删除"})
}

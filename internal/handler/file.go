package handler

import (
	"errors"
	"path/filepath"
	"strconv"

	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/repository"
	"github.com/kyangconn/goleaf/internal/service"
	resp "github.com/kyangconn/goleaf/pkg/response"
)

// FileHandler 文件管理 HTTP 处理器
type FileHandler struct {
	svc service.FileService
}

// NewFileHandler 创建文件管理处理器
func NewFileHandler(svc service.FileService) *FileHandler {
	return &FileHandler{svc: svc}
}

// ---- 请求结构体 ----

type createFileReq struct {
	Name     string `json:"name" binding:"required,min=1,max=255"`
	ParentID *uint  `json:"parent_id"` // null = 根目录
	IsDir    bool   `json:"is_dir"`
}

type updateContentReq struct {
	Content string `json:"content" binding:"required"`
}

type renameFileReq struct {
	Name string `json:"name" binding:"required,min=1,max=255"`
}

// GetTree 获取项目文件树
// GET /api/projects/:id/files
func (h *FileHandler) GetTree(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		resp.BadRequest(c, "无效的项目 ID")
		return
	}

	tree, err := h.svc.GetTree(uint(projectID), userID)
	if err != nil {
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权访问该项目")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, tree)
}

// GetContent 获取单个文件内容
// GET /api/projects/:id/files/:fileId
func (h *FileHandler) GetContent(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		resp.BadRequest(c, "无效的 ID 参数")
		return
	}

	file, err := h.svc.GetContent(fileID, projectID, userID)
	if err != nil {
		if errors.Is(err, repository.ErrFileNotFound) {
			resp.NotFound(c, "文件不存在")
			return
		}
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权访问该文件")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, file)
}

// Create 在项目中创建文件或文件夹
// POST /api/projects/:id/files
func (h *FileHandler) Create(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		resp.BadRequest(c, "无效的项目 ID")
		return
	}

	var req createFileReq
	if err := c.ShouldBindJSON(&req); err != nil {
		resp.BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	file, err := h.svc.Create(uint(projectID), userID, req.Name, req.ParentID, req.IsDir)
	if err != nil {
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权在该项目中创建文件")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.Created(c, file)
}

// UpdateContent 更新文件内容
// PUT /api/projects/:id/files/:fileId
func (h *FileHandler) UpdateContent(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		resp.BadRequest(c, "无效的 ID 参数")
		return
	}

	var req updateContentReq
	if err := c.ShouldBindJSON(&req); err != nil {
		resp.BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	file, err := h.svc.UpdateContent(fileID, projectID, userID, req.Content)
	if err != nil {
		if errors.Is(err, repository.ErrFileNotFound) {
			resp.NotFound(c, "文件不存在")
			return
		}
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权修改该文件")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, file)
}

// Rename 重命名文件或文件夹
// PATCH /api/projects/:id/files/:fileId/rename
func (h *FileHandler) Rename(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		resp.BadRequest(c, "无效的 ID 参数")
		return
	}

	var req renameFileReq
	if err := c.ShouldBindJSON(&req); err != nil {
		resp.BadRequest(c, "请求参数无效: "+err.Error())
		return
	}

	file, err := h.svc.Rename(fileID, projectID, userID, req.Name)
	if err != nil {
		if errors.Is(err, repository.ErrFileNotFound) {
			resp.NotFound(c, "文件不存在")
			return
		}
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权重命名该文件")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, file)
}

// Delete 删除文件或文件夹
// DELETE /api/projects/:id/files/:fileId
func (h *FileHandler) Delete(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		resp.BadRequest(c, "无效的 ID 参数")
		return
	}

	if err := h.svc.Delete(fileID, projectID, userID); err != nil {
		if errors.Is(err, repository.ErrFileNotFound) {
			resp.NotFound(c, "文件不存在")
			return
		}
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权删除该文件")
			return
		}
		resp.Fail(c, 500, err)
		return
	}

	resp.OK(c, gin.H{"message": "文件已删除"})
}

// Compile 编译 LaTeX 项目并返回 PDF
// POST /api/projects/:id/compile
func (h *FileHandler) Compile(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		resp.BadRequest(c, "无效的项目 ID")
		return
	}

	pdfPath, logOutput, err := h.svc.Compile(uint(projectID), userID)
	if err != nil {
		if errors.Is(err, repository.ErrForbidden) {
			resp.Forbidden(c, "无权编译该项目")
			return
		}
		resp.Fail(c, 500, err)
		// 额外附加编译日志供调试
		_ = logOutput
		return
	}

	c.Header("Content-Type", "application/pdf")
	c.Header("Content-Disposition", "inline; filename=\""+filepath.Base(pdfPath)+"\"")
	c.File(pdfPath)
}

// parseProjectAndFileID 从 URL 参数中解析 project_id 和 file_id
func parseProjectAndFileID(c *gin.Context) (projectID, fileID uint, err error) {
	pid, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		return 0, 0, err
	}
	fid, err := strconv.ParseUint(c.Param("fileId"), 10, 64)
	if err != nil {
		return 0, 0, err
	}
	return uint(pid), uint(fid), nil
}

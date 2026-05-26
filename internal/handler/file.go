package handler

import (
	"net/http"
	"path/filepath"
	"strconv"

	"github.com/gin-gonic/gin"

	"github.com/kyangconn/goleaf/internal/service"
)

// FileHandler 文件管理 HTTP 处理器
type FileHandler struct {
	svc *service.FileService
}

// NewFileHandler 创建文件管理处理器
func NewFileHandler(svc *service.FileService) *FileHandler {
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
func (h *FileHandler) GetTree(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的项目 ID"})
		return
	}

	tree, err := h.svc.GetTree(uint(projectID), userID)
	if err != nil {
		c.JSON(http.StatusForbidden, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, tree)
}

// GetContent 获取单个文件内容
func (h *FileHandler) GetContent(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	file, err := h.svc.GetContent(fileID, projectID, userID)
	if err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, file)
}

// Create 在项目中创建文件或文件夹
func (h *FileHandler) Create(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的项目 ID"})
		return
	}

	var req createFileReq
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "请求参数无效: " + err.Error()})
		return
	}

	file, err := h.svc.Create(uint(projectID), userID, req.Name, req.ParentID, req.IsDir)
	if err != nil {
		c.JSON(http.StatusForbidden, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusCreated, file)
}

// UpdateContent 更新文件内容
func (h *FileHandler) UpdateContent(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var req updateContentReq
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "请求参数无效: " + err.Error()})
		return
	}

	file, err := h.svc.UpdateContent(fileID, projectID, userID, req.Content)
	if err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, file)
}

// Rename 重命名文件或文件夹
func (h *FileHandler) Rename(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var req renameFileReq
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "请求参数无效: " + err.Error()})
		return
	}

	file, err := h.svc.Rename(fileID, projectID, userID, req.Name)
	if err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, file)
}

// Delete 删除文件或文件夹
func (h *FileHandler) Delete(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, fileID, err := parseProjectAndFileID(c)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	if err := h.svc.Delete(fileID, projectID, userID); err != nil {
		c.JSON(http.StatusForbidden, gin.H{"error": err.Error()})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "文件已删除"})
}

// Compile 编译 LaTeX 项目并返回 PDF
func (h *FileHandler) Compile(c *gin.Context) {
	userID := c.GetUint("userID")
	projectID, err := strconv.ParseUint(c.Param("id"), 10, 64)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "无效的项目 ID"})
		return
	}

	pdfPath, logOutput, err := h.svc.Compile(uint(projectID), userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{
			"error": err.Error(),
			"log":   logOutput,
		})
		return
	}

	// 返回 PDF 文件
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

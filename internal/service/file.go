package service

import (
	"bytes"
	"context"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"

	"gorm.io/gorm"

	"github.com/kyangconn/goleaf/internal/model"
)

// FileService 文件管理与 LaTeX 编译服务
type FileService struct {
	db        *gorm.DB
	compiler  string // LaTeX 编译器命令
	compileTO int    // 编译超时秒数
	outputDir string // 编译输出目录
}

// NewFileService 创建文件服务实例
func NewFileService(db *gorm.DB, compiler string, timeout int, outputDir string) *FileService {
	return &FileService{
		db:        db,
		compiler:  compiler,
		compileTO: timeout,
		outputDir: outputDir,
	}
}

// GetTree 获取项目文件树 (带权限校验)
func (s *FileService) GetTree(projectID, userID uint) ([]*model.File, error) {
	// 校验用户对该项目有权限
	if err := s.checkProjectAccess(projectID, userID); err != nil {
		return nil, err
	}

	var flat []model.File
	if err := s.db.Where("project_id = ?", projectID).
		Order("is_dir DESC, name ASC").Find(&flat).Error; err != nil {
		return nil, fmt.Errorf("查询文件列表失败: %w", err)
	}

	// 构建树形结构
	return buildTree(flat, nil), nil
}

// GetContent 获取单个文件内容 (带权限校验)
func (s *FileService) GetContent(fileID, projectID, userID uint) (*model.File, error) {
	if err := s.checkProjectAccess(projectID, userID); err != nil {
		return nil, err
	}

	var file model.File
	if err := s.db.Where("id = ? AND project_id = ?", fileID, projectID).First(&file).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, errors.New("文件不存在")
		}
		return nil, fmt.Errorf("查询文件失败: %w", err)
	}
	return &file, nil
}

// Create 在项目中创建文件或文件夹
func (s *FileService) Create(projectID, userID uint, name string, parentID *uint, isDir bool) (*model.File, error) {
	if err := s.checkProjectAccess(projectID, userID); err != nil {
		return nil, err
	}

	file := &model.File{
		ProjectID: projectID,
		ParentID:  parentID,
		Name:      name,
		IsDir:     isDir,
		Content:   "",
	}

	if err := s.db.Create(file).Error; err != nil {
		return nil, fmt.Errorf("创建文件失败: %w", err)
	}
	return file, nil
}

// UpdateContent 更新文件内容
func (s *FileService) UpdateContent(fileID, projectID, userID uint, content string) (*model.File, error) {
	if err := s.checkProjectAccess(projectID, userID); err != nil {
		return nil, err
	}

	var file model.File
	if err := s.db.Where("id = ? AND project_id = ? AND is_dir = ?", fileID, projectID, false).First(&file).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, errors.New("文件不存在或为文件夹")
		}
		return nil, fmt.Errorf("查询文件失败: %w", err)
	}

	file.Content = content
	if err := s.db.Save(&file).Error; err != nil {
		return nil, fmt.Errorf("保存文件失败: %w", err)
	}
	return &file, nil
}

// Rename 重命名文件或文件夹
func (s *FileService) Rename(fileID, projectID, userID uint, newName string) (*model.File, error) {
	if err := s.checkProjectAccess(projectID, userID); err != nil {
		return nil, err
	}

	var file model.File
	if err := s.db.Where("id = ? AND project_id = ?", fileID, projectID).First(&file).Error; err != nil {
		return nil, fmt.Errorf("文件不存在: %w", err)
	}

	file.Name = newName
	if err := s.db.Save(&file).Error; err != nil {
		return nil, fmt.Errorf("重命名失败: %w", err)
	}
	return &file, nil
}

// Delete 删除文件或文件夹 (递归删除子节点)
func (s *FileService) Delete(fileID, projectID, userID uint) error {
	if err := s.checkProjectAccess(projectID, userID); err != nil {
		return err
	}

	// 递归收集所有待删除的 ID
	ids, err := s.collectDescendantIDs(projectID, fileID)
	if err != nil {
		return err
	}
	ids = append(ids, fileID)

	return s.db.Where("id IN ?", ids).Delete(&model.File{}).Error
}

// Compile 编译 LaTeX 工程，返回 PDF 路径和编译日志
func (s *FileService) Compile(projectID, userID uint) (pdfPath string, logOutput string, err error) {
	if err := s.checkProjectAccess(projectID, userID); err != nil {
		return "", "", err
	}

	// 1. 获取主文件 (优先 main.tex，否则第一个 .tex 文件)
	mainFile, err := s.findMainFile(projectID)
	if err != nil {
		return "", "", err
	}

	// 2. 创建临时编译目录
	workDir := filepath.Join(s.outputDir, fmt.Sprintf("project_%d", projectID))
	if err := os.MkdirAll(workDir, 0755); err != nil {
		return "", "", fmt.Errorf("创建编译目录失败: %w", err)
	}

	// 3. 将所有项目文件写入工作目录
	if err := s.writeProjectFiles(projectID, workDir); err != nil {
		return "", "", err
	}

	// 4. 运行 LaTeX 编译器
	ctx, cancel := context.WithTimeout(context.Background(), time.Duration(s.compileTO)*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, s.compiler,
		"-interaction=nonstopmode",
		"-output-directory="+workDir,
		mainFile,
	)
	cmd.Dir = workDir

	var stdout, stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	runErr := cmd.Run()
	logOutput = stdout.String()
	if stderr.Len() > 0 {
		logOutput += "\n[stderr]\n" + stderr.String()
	}

	if runErr != nil {
		if ctx.Err() == context.DeadlineExceeded {
			return "", logOutput, errors.New("编译超时")
		}
		// 编译警告/错误不视为致命错误，仍然尝试返回 PDF
	}

	// 5. 查找生成的 PDF
	pdfName := strings.TrimSuffix(mainFile, ".tex") + ".pdf"
	pdfPath = filepath.Join(workDir, pdfName)
	if _, statErr := os.Stat(pdfPath); os.IsNotExist(statErr) {
		return "", logOutput, fmt.Errorf("编译失败，未生成 PDF:\n%s", logOutput)
	}

	return pdfPath, logOutput, nil
}

// checkProjectAccess 校验用户对项目的访问权限 (拥有者或协作者)
func (s *FileService) checkProjectAccess(projectID, userID uint) error {
	var project model.Project
	if err := s.db.First(&project, projectID).Error; err != nil {
		return errors.New("项目不存在")
	}
	if project.OwnerID == userID {
		return nil
	}
	// 预留: 检查协作者表
	var count int64
	s.db.Model(&model.Collaborator{}).
		Where("project_id = ? AND user_id = ?", projectID, userID).
		Count(&count)
	if count == 0 {
		return errors.New("无权限访问该项目")
	}
	return nil
}

// findMainFile 寻找主 .tex 文件 (优先 main.tex)
func (s *FileService) findMainFile(projectID uint) (string, error) {
	var files []model.File
	if err := s.db.Where("project_id = ? AND is_dir = ?", projectID, false).
		Find(&files).Error; err != nil {
		return "", fmt.Errorf("查询项目文件失败: %w", err)
	}

	if len(files) == 0 {
		return "", errors.New("项目中没有任何 .tex 文件")
	}

	// 优先 main.tex
	for _, f := range files {
		if strings.EqualFold(f.Name, "main.tex") {
			return f.Name, nil
		}
	}
	// 否则取第一个 .tex 文件
	for _, f := range files {
		if strings.HasSuffix(strings.ToLower(f.Name), ".tex") {
			return f.Name, nil
		}
	}

	return "", errors.New("项目中找不到 .tex 文件")
}

// writeProjectFiles 将项目的所有文件写入磁盘目录 (递归)
func (s *FileService) writeProjectFiles(projectID uint, baseDir string) error {
	var files []model.File
	if err := s.db.Where("project_id = ?", projectID).Find(&files).Error; err != nil {
		return err
	}

	// 构建 ID -> File 映射，用于路径拼接
	fileMap := make(map[uint]*model.File)
	for i := range files {
		fileMap[files[i].ID] = &files[i]
	}

	for _, f := range files {
		if f.IsDir {
			continue // 目录稍后由文件自动创建
		}

		relPath := s.buildRelativePath(f.ID, fileMap)
		fullPath := filepath.Join(baseDir, relPath)

		if err := os.MkdirAll(filepath.Dir(fullPath), 0755); err != nil {
			return fmt.Errorf("创建目录失败: %w", err)
		}
		if err := os.WriteFile(fullPath, []byte(f.Content), 0644); err != nil {
			return fmt.Errorf("写入文件失败: %w", err)
		}
	}
	return nil
}

// buildRelativePath 根据父级关系构建文件相对路径
func (s *FileService) buildRelativePath(fileID uint, fileMap map[uint]*model.File) string {
	var parts []string
	currentID := &fileID

	for {
		f, ok := fileMap[*currentID]
		if !ok {
			break
		}
		parts = append([]string{f.Name}, parts...)
		if f.ParentID == nil {
			break
		}
		currentID = f.ParentID
	}

	return filepath.Join(parts...)
}

// collectDescendantIDs 递归收集某节点的所有子孙 ID
func (s *FileService) collectDescendantIDs(projectID, parentID uint) ([]uint, error) {
	var children []model.File
	if err := s.db.Where("project_id = ? AND parent_id = ?", projectID, parentID).Find(&children).Error; err != nil {
		return nil, err
	}

	var ids []uint
	for _, child := range children {
		ids = append(ids, child.ID)
		subIDs, err := s.collectDescendantIDs(projectID, child.ID)
		if err != nil {
			return nil, err
		}
		ids = append(ids, subIDs...)
	}
	return ids, nil
}

// buildTree 将扁平文件列表构建为树形结构
func buildTree(files []model.File, parentID *uint) []*model.File {
	var result []*model.File
	for i := range files {
		f := &files[i]
		// 比较 parent_id (处理 nil 的情况)
		if (parentID == nil && f.ParentID == nil) || (parentID != nil && f.ParentID != nil && *f.ParentID == *parentID) {
			f.Children = buildTree(files, &f.ID)
			result = append(result, f)
		}
	}
	return result
}

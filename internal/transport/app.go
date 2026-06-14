package transport

import (
	"context"
	"fmt"
	"os"
	"time"

	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/factory"
	pklog "github.com/kyangconn/goleaf/internal/log"
)

type App struct {
	ctx context.Context
	fa  *factory.App
}

func New() *App { return &App{} }

func (a *App) Startup(ctx context.Context) {
	a.ctx = ctx
	fa, err := factory.New()
	if err != nil {
		pklog.Fatalf("初始化失败: %v", err)
	}
	a.fa = fa
}

// ProjectDTO 是暴露给 Wails 前端的项目结构。
type ProjectDTO struct {
	ID        uint   `json:"id"`
	Name      string `json:"name"`
	CreatedAt string `json:"created_at"`
	UpdatedAt string `json:"updated_at"`
}

// FileDTO 是暴露给 Wails 前端的文件结构。
type FileDTO struct {
	ID        uint       `json:"id"`
	ProjectID uint       `json:"project_id"`
	ParentID  *uint      `json:"parent_id"`
	Name      string     `json:"name"`
	IsDir     bool       `json:"is_dir"`
	Content   string     `json:"content,omitempty"`
	CreatedAt string     `json:"created_at"`
	UpdatedAt string     `json:"updated_at"`
	Children  []*FileDTO `json:"children,omitempty"`
}

func formatTime(t time.Time) string {
	if t.IsZero() {
		return ""
	}
	return t.Format(time.RFC3339)
}

func toProjectDTO(project *domain.Project) *ProjectDTO {
	if project == nil {
		return nil
	}
	return &ProjectDTO{
		ID:        project.ID,
		Name:      project.Name,
		CreatedAt: formatTime(project.CreatedAt),
		UpdatedAt: formatTime(project.UpdatedAt),
	}
}

func toProjectDTOs(projects []domain.Project) []ProjectDTO {
	result := make([]ProjectDTO, 0, len(projects))
	for i := range projects {
		result = append(result, *toProjectDTO(&projects[i]))
	}
	return result
}

func toFileDTO(file *domain.File) *FileDTO {
	if file == nil {
		return nil
	}
	result := &FileDTO{
		ID:        file.ID,
		ProjectID: file.ProjectID,
		ParentID:  file.ParentID,
		Name:      file.Name,
		IsDir:     file.IsDir,
		Content:   file.Content,
		CreatedAt: formatTime(file.CreatedAt),
		UpdatedAt: formatTime(file.UpdatedAt),
	}
	if len(file.Children) > 0 {
		result.Children = make([]*FileDTO, 0, len(file.Children))
		for _, child := range file.Children {
			result.Children = append(result.Children, toFileDTO(child))
		}
	}
	return result
}

func toFileDTOs(files []*domain.File) []*FileDTO {
	result := make([]*FileDTO, 0, len(files))
	for _, file := range files {
		result = append(result, toFileDTO(file))
	}
	return result
}

// === 项目 ===
func (a *App) ListProjects() ([]ProjectDTO, error) {
	projects, err := a.fa.ProjectSvc.List()
	if err != nil {
		return nil, err
	}
	return toProjectDTOs(projects), nil
}
func (a *App) GetProject(id uint) (*ProjectDTO, error) {
	project, err := a.fa.ProjectSvc.GetByID(id)
	if err != nil {
		return nil, err
	}
	return toProjectDTO(project), nil
}
func (a *App) CreateProject(name string) (*ProjectDTO, error) {
	project, err := a.fa.ProjectSvc.Create(name)
	if err != nil {
		return nil, err
	}
	return toProjectDTO(project), nil
}
func (a *App) UpdateProject(id uint, name string) (*ProjectDTO, error) {
	project, err := a.fa.ProjectSvc.Update(id, name)
	if err != nil {
		return nil, err
	}
	return toProjectDTO(project), nil
}
func (a *App) DeleteProject(id uint) error { return a.fa.ProjectSvc.Delete(id) }

// === 文件 ===
func (a *App) GetFileTree(projectID uint) ([]*FileDTO, error) {
	files, err := a.fa.FileSvc.GetTree(projectID)
	if err != nil {
		return nil, err
	}
	return toFileDTOs(files), nil
}
func (a *App) GetFileContent(projectID, fileID uint) (*FileDTO, error) {
	file, err := a.fa.FileSvc.GetContent(fileID, projectID)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}
func (a *App) CreateFile(projectID uint, name string, parentID *uint, isDir bool) (*FileDTO, error) {
	file, err := a.fa.FileSvc.Create(projectID, name, parentID, isDir)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}
func (a *App) UpdateFileContent(projectID, fileID uint, content string) (*FileDTO, error) {
	file, err := a.fa.FileSvc.UpdateContent(fileID, projectID, content)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}
func (a *App) RenameFile(projectID, fileID uint, newName string) (*FileDTO, error) {
	file, err := a.fa.FileSvc.Rename(fileID, projectID, newName)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}
func (a *App) DeleteFile(projectID, fileID uint) error { return a.fa.FileSvc.Delete(fileID, projectID) }

// === 编译 ===
type CompileResult struct {
	PDF []byte `json:"pdf"`
	Log string `json:"log"`
}

func (a *App) CompileProject(projectID uint) (*CompileResult, error) {
	pdfPath, logOutput, err := a.fa.FileSvc.Compile(projectID)
	if err != nil {
		return nil, err
	}
	pdfBytes, err := os.ReadFile(pdfPath)
	if err != nil {
		return nil, fmt.Errorf("读取 PDF 失败: %w", err)
	}
	return &CompileResult{PDF: pdfBytes, Log: logOutput}, nil
}

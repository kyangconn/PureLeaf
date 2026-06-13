package transport

import (
	"context"
	"fmt"
	"os"

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

// === 项目 ===
func (a *App) ListProjects() ([]domain.Project, error)     { return a.fa.ProjectSvc.List() }
func (a *App) GetProject(id uint) (*domain.Project, error) { return a.fa.ProjectSvc.GetByID(id) }
func (a *App) CreateProject(name string) (*domain.Project, error) {
	return a.fa.ProjectSvc.Create(name)
}
func (a *App) UpdateProject(id uint, name string) (*domain.Project, error) {
	return a.fa.ProjectSvc.Update(id, name)
}
func (a *App) DeleteProject(id uint) error { return a.fa.ProjectSvc.Delete(id) }

// === 文件 ===
func (a *App) GetFileTree(projectID uint) ([]*domain.File, error) {
	return a.fa.FileSvc.GetTree(projectID)
}
func (a *App) GetFileContent(projectID, fileID uint) (*domain.File, error) {
	return a.fa.FileSvc.GetContent(fileID, projectID)
}
func (a *App) CreateFile(projectID uint, name string, parentID *uint, isDir bool) (*domain.File, error) {
	return a.fa.FileSvc.Create(projectID, name, parentID, isDir)
}
func (a *App) UpdateFileContent(projectID, fileID uint, content string) (*domain.File, error) {
	return a.fa.FileSvc.UpdateContent(fileID, projectID, content)
}
func (a *App) RenameFile(projectID, fileID uint, newName string) (*domain.File, error) {
	return a.fa.FileSvc.Rename(fileID, projectID, newName)
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

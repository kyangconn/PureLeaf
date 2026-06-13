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
func (a *App) ListProjects() ([]domain.Project, error)              { return a.fa.ProjectSvc.ListByUser(1) }
func (a *App) GetProject(id uint) (*domain.Project, error)          { return a.fa.ProjectSvc.GetByID(id, 1) }
func (a *App) CreateProject(name string) (*domain.Project, error)   { return a.fa.ProjectSvc.Create(name, 1) }
func (a *App) UpdateProject(id uint, name string) (*domain.Project, error) { return a.fa.ProjectSvc.Update(id, 1, name) }
func (a *App) DeleteProject(id uint) error                          { return a.fa.ProjectSvc.Delete(id, 1) }

// === 文件 ===
func (a *App) GetFileTree(projectID uint) ([]*domain.File, error)   { return a.fa.FileSvc.GetTree(projectID, 1) }
func (a *App) GetFileContent(projectID, fileID uint) (*domain.File, error) { return a.fa.FileSvc.GetContent(fileID, projectID, 1) }
func (a *App) CreateFile(projectID uint, name string, parentID *uint, isDir bool) (*domain.File, error) {
	return a.fa.FileSvc.Create(projectID, 1, name, parentID, isDir)
}
func (a *App) UpdateFileContent(projectID, fileID uint, content string) (*domain.File, error) {
	return a.fa.FileSvc.UpdateContent(fileID, projectID, 1, content)
}
func (a *App) RenameFile(projectID, fileID uint, newName string) (*domain.File, error) {
	return a.fa.FileSvc.Rename(fileID, projectID, 1, newName)
}
func (a *App) DeleteFile(projectID, fileID uint) error { return a.fa.FileSvc.Delete(fileID, projectID, 1) }

// === 编译 ===
type CompileResult struct {
	PDF []byte `json:"pdf"`
	Log string `json:"log"`
}

func (a *App) CompileProject(projectID uint) (*CompileResult, error) {
	pdfPath, logOutput, err := a.fa.FileSvc.Compile(projectID, 1)
	if err != nil {
		return nil, err
	}
	pdfBytes, err := os.ReadFile(pdfPath)
	if err != nil {
		return nil, fmt.Errorf("读取 PDF 失败: %w", err)
	}
	return &CompileResult{PDF: pdfBytes, Log: logOutput}, nil
}

// === 系统状态 ===
func (a *App) HasUsers() bool {
	has, _ := a.fa.UserSvc.HasUsers()
	return has
}
func (a *App) GetCurrentUser() (*domain.User, error) {
	return a.fa.UserSvc.GetByID(1)
}

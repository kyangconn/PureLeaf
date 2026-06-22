package service

import (
	"fmt"
	"path/filepath"

	"github.com/kyangconn/goleaf/internal/repository"
)

// ---- SyncTeX 正反向同步 ----

// mainFileRel 返回项目主 .tex 文件名（不含路径），用于编译产物定位。
func (s *fileService) mainFileRel(projectID uint) (string, error) {
	name, err := s.findMainFile(projectID)
	if err != nil {
		return "", err
	}
	return name, nil
}

// SynctexForward 正向同步：源码行 → PDF 页面位置。
// input 为 TeX 源文件相对项目目录的路径（或文件名）。
func (s *fileService) SynctexForward(projectID uint, input string, line, column int) (*SynctexViewResult, error) {
	mainFile, err := s.mainFileRel(projectID)
	if err != nil {
		return nil, err
	}
	workDir := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID))
	pdfName := stripTeXExt(mainFile) + ".pdf"
	return SynctexView(workDir, input, line, column, pdfName)
}

// SynctexInverse 反向同步：PDF 页面坐标 → 源码行。
// page 为 1-based，x/y 为页面内坐标（big point, 72 dpi）。
func (s *fileService) SynctexInverse(projectID uint, page int, x, y float64) (*SynctexEditResult, error) {
	mainFile, err := s.mainFileRel(projectID)
	if err != nil {
		return nil, err
	}
	// 确认文件仍在项目中（避免对已删项目查询）
	if _, err := s.projectRepo.FindByID(projectID); err != nil {
		return nil, repository.ErrProjectNotFound
	}
	workDir := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID))
	pdfName := stripTeXExt(mainFile) + ".pdf"
	return SynctexEdit(workDir, page, x, y, pdfName)
}

// stripTeXExt 去掉 .tex 后缀（大小写不敏感）。
func stripTeXExt(name string) string {
	if len(name) > 4 && (name[len(name)-4:] == ".tex" || name[len(name)-4:] == ".TEX") {
		return name[:len(name)-4]
	}
	return name
}

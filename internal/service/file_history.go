package service

import (
	"fmt"
	"os/exec"
	"path/filepath"
	"runtime"

	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/repository"
)

// ---- 文件历史与快照查询 ----

// GetFileHistory 返回某文件的历史版本列表（按时间倒序）。
func (s *fileService) GetFileHistory(fileID, projectID uint, limit int) ([]domain.FileRevision, error) {
	// 校验 fileID 属于 projectID
	file, err := s.fileRepo.FindByID(fileID)
	if err != nil {
		return nil, err
	}
	if file.ProjectID != projectID {
		return nil, repository.ErrFileNotFound
	}
	return s.revRepo.FindByFileID(fileID, limit)
}

// GetProjectHistory 返回项目级历史版本列表（聚合所有文件，按时间倒序）。
func (s *fileService) GetProjectHistory(projectID uint, limit int) ([]domain.FileRevision, error) {
	return s.revRepo.FindByProjectID(projectID, limit)
}

// GetProjectSnapshots 返回项目的快照记录列表（按时间倒序）。
func (s *fileService) GetProjectSnapshots(projectID uint, limit int) ([]domain.ProjectSnapshot, error) {
	return s.snapRepo.FindByProjectID(projectID, limit)
}

// GetRevisionContent 读取某个历史版本的内容。
func (s *fileService) GetRevisionContent(revID uint) (string, error) {
	rev, err := s.revRepo.FindByID(revID)
	if err != nil {
		return "", err
	}
	data, err := s.blobStore.Get(rev.BlobPath)
	if err != nil {
		return "", err
	}
	return string(data), nil
}

// DiffRevisions 比较两个历史版本，返回 unified diff 文本。
// 版本顺序（a/b）不影响正确性，只影响 +/- 方向。
func (s *fileService) DiffRevisions(revA, revB uint) (string, error) {
	ra, err := s.revRepo.FindByID(revA)
	if err != nil {
		return "", err
	}
	rb, err := s.revRepo.FindByID(revB)
	if err != nil {
		return "", err
	}
	old, err := s.blobStore.Get(ra.BlobPath)
	if err != nil {
		return "", err
	}
	newText, err := s.blobStore.Get(rb.BlobPath)
	if err != nil {
		return "", err
	}
	headerOld := fmt.Sprintf("r%d (%s, %d bytes)", ra.ID, ra.FilePath, ra.Size)
	headerNew := fmt.Sprintf("r%d (%s, %d bytes)", rb.ID, rb.FilePath, rb.Size)
	return ComputeDiff(headerOld, headerNew, string(old), string(newText)), nil
}

// ---- 系统 ----

// OpenProjectFolder 在系统文件管理器中打开项目目录。
//
// 平台行为：
//   - Windows: explorer 打开项目目录
//   - macOS:   Finder 打开项目目录
//   - Linux:   xdg-open 打开项目目录（AppImage 同样适用，xdg-open 是系统命令）
func (s *fileService) OpenProjectFolder(projectID uint) error {
	projDir := filepath.Join(s.dataDir, fmt.Sprintf("%d", projectID))
	return openInFileManager(projDir)
}

// openInFileManager 跨平台打开路径所在的文件夹。
func openInFileManager(path string) error {
	switch runtime.GOOS {
	case "windows":
		return exec.Command("explorer", path).Start()
	case "darwin":
		return exec.Command("open", path).Start()
	default:
		// Linux / *BSD：xdg-open 是桌面环境无关的标准入口。
		// AppImage 不会影响这里——它调用的是宿主系统的 xdg-open。
		return exec.Command("xdg-open", path).Start()
	}
}

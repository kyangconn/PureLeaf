package bindings

import (
	"fmt"
	"os"

	"github.com/kyangconn/goleaf/internal/service"
)

// FileService exposes file and compile operations to the Wails frontend.
type FileService struct {
	files service.FileService
}

func NewFileService(files service.FileService) *FileService {
	return &FileService{files: files}
}

func (s *FileService) GetFileTree(projectID uint) ([]*FileDTO, error) {
	files, err := s.files.GetTree(projectID)
	if err != nil {
		return nil, err
	}
	return toFileDTOs(files), nil
}

func (s *FileService) GetFileContent(projectID, fileID uint) (*FileDTO, error) {
	file, err := s.files.GetContent(fileID, projectID)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}

func (s *FileService) CreateFile(projectID uint, name string, parentID *uint, isDir bool) (*FileDTO, error) {
	file, err := s.files.Create(projectID, name, parentID, isDir)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}

func (s *FileService) UpdateFileContent(projectID, fileID uint, content string) (*FileDTO, error) {
	file, err := s.files.UpdateContent(fileID, projectID, content)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}

func (s *FileService) RenameFile(projectID, fileID uint, newName string) (*FileDTO, error) {
	file, err := s.files.Rename(fileID, projectID, newName)
	if err != nil {
		return nil, err
	}
	return toFileDTO(file), nil
}

func (s *FileService) DeleteFile(projectID, fileID uint) error {
	return s.files.Delete(fileID, projectID)
}

func (s *FileService) CompileProject(projectID uint) (*CompileResult, error) {
	pdfPath, logOutput, err := s.files.Compile(projectID)
	if err != nil {
		return nil, err
	}
	pdfBytes, err := os.ReadFile(pdfPath)
	if err != nil {
		return nil, fmt.Errorf("读取 PDF 失败: %w", err)
	}
	return &CompileResult{PDF: pdfBytes, Log: logOutput}, nil
}

func (s *FileService) GetFileHistory(projectID, fileID uint) ([]FileRevisionDTO, error) {
	revs, err := s.files.GetFileHistory(fileID, projectID, 0)
	if err != nil {
		return nil, err
	}
	return toFileRevisionDTOs(revs), nil
}

func (s *FileService) GetProjectHistory(projectID uint) ([]FileRevisionDTO, error) {
	revs, err := s.files.GetProjectHistory(projectID, 100)
	if err != nil {
		return nil, err
	}
	return toFileRevisionDTOs(revs), nil
}

func (s *FileService) GetProjectSnapshots(projectID uint) ([]ProjectSnapshotDTO, error) {
	snaps, err := s.files.GetProjectSnapshots(projectID, 100)
	if err != nil {
		return nil, err
	}
	return toProjectSnapshotDTOs(snaps), nil
}

func (s *FileService) GetRevisionContent(revID uint) (*RevisionContentDTO, error) {
	content, err := s.files.GetRevisionContent(revID)
	if err != nil {
		return nil, err
	}
	return &RevisionContentDTO{Content: content}, nil
}

func (s *FileService) DiffRevisions(revA, revB uint) (*DiffResultDTO, error) {
	diff, err := s.files.DiffRevisions(revA, revB)
	if err != nil {
		return nil, err
	}
	return &DiffResultDTO{Diff: diff}, nil
}

func (s *FileService) OpenProjectFolder(projectID uint) error {
	return s.files.OpenProjectFolder(projectID)
}

func (s *FileService) SynctexForward(projectID uint, input string, line, column int) (*SynctexViewDTO, error) {
	result, err := s.files.SynctexForward(projectID, input, line, column)
	if err != nil {
		return nil, err
	}
	return toSynctexViewDTO(result), nil
}

func (s *FileService) SynctexInverse(projectID uint, page int, x, y float64) (*SynctexEditDTO, error) {
	result, err := s.files.SynctexInverse(projectID, page, x, y)
	if err != nil {
		return nil, err
	}
	return toSynctexEditDTO(result), nil
}

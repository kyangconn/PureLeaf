package bindings

import (
	"time"

	"github.com/kyangconn/goleaf/internal/domain"
)

// ProjectDTO is the project shape exposed to Wails bindings.
type ProjectDTO struct {
	ID        uint   `json:"id"`
	Name      string `json:"name"`
	CreatedAt string `json:"created_at"`
	UpdatedAt string `json:"updated_at"`
}

// FileDTO is the file shape exposed to Wails bindings.
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

// CompileResult contains the PDF bytes and compiler log returned to the UI.
type CompileResult struct {
	PDF []byte `json:"pdf"`
	Log string `json:"log"`
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

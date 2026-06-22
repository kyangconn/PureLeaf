package bindings

import (
	"time"

	"github.com/kyangconn/goleaf/internal/domain"
	"github.com/kyangconn/goleaf/internal/service"
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

// FileRevisionDTO 是文件历史版本的对外结构。
type FileRevisionDTO struct {
	ID          uint   `json:"id"`
	ProjectID   uint   `json:"project_id"`
	FileID      uint   `json:"file_id"`
	FilePath    string `json:"file_path"`
	ContentHash string `json:"content_hash"`
	Size        int64  `json:"size"`
	Reason      string `json:"reason"`
	SnapshotID  *uint  `json:"snapshot_id,omitempty"`
	CreatedAt   string `json:"created_at"`
}

// ProjectSnapshotDTO 是项目快照的对外结构。
type ProjectSnapshotDTO struct {
	ID         uint   `json:"id"`
	ProjectID  uint   `json:"project_id"`
	Reason     string `json:"reason"`
	FileCount  int    `json:"file_count"`
	TotalSize  int64  `json:"total_size"`
	SnapshotOf string `json:"snapshot_of"`
	CreatedAt  string `json:"created_at"`
}

// RevisionContentDTO 包装某个历史版本的完整内容。
type RevisionContentDTO struct {
	Content string `json:"content"`
}

// DiffResultDTO 包装两个历史版本间的 unified diff 文本。
type DiffResultDTO struct {
	Diff string `json:"diff"`
}

// SynctexViewDTO 是正向同步（源码 → PDF）结果。
type SynctexViewDTO struct {
	Page   int     `json:"page"`
	X      float64 `json:"x"`
	Y      float64 `json:"y"`
	H      float64 `json:"h"`
	V      float64 `json:"v"`
	W      float64 `json:"w"`
	Height float64 `json:"height"`
}

// SynctexEditDTO 是反向同步（PDF → 源码）结果。
type SynctexEditDTO struct {
	Input  string `json:"input"`
	Line   int    `json:"line"`
	Column int    `json:"column"`
	Offset int    `json:"offset"`
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

func toFileRevisionDTO(rev *domain.FileRevision) *FileRevisionDTO {
	if rev == nil {
		return nil
	}
	return &FileRevisionDTO{
		ID:          rev.ID,
		ProjectID:   rev.ProjectID,
		FileID:      rev.FileID,
		FilePath:    rev.FilePath,
		ContentHash: rev.ContentHash,
		Size:        rev.Size,
		Reason:      rev.Reason,
		SnapshotID:  rev.SnapshotID,
		CreatedAt:   formatTime(rev.CreatedAt),
	}
}

func toFileRevisionDTOs(revs []domain.FileRevision) []FileRevisionDTO {
	result := make([]FileRevisionDTO, 0, len(revs))
	for i := range revs {
		result = append(result, *toFileRevisionDTO(&revs[i]))
	}
	return result
}

func toProjectSnapshotDTO(snap *domain.ProjectSnapshot) *ProjectSnapshotDTO {
	if snap == nil {
		return nil
	}
	return &ProjectSnapshotDTO{
		ID:         snap.ID,
		ProjectID:  snap.ProjectID,
		Reason:     snap.Reason,
		FileCount:  snap.FileCount,
		TotalSize:  snap.TotalSize,
		SnapshotOf: snap.SnapshotOf,
		CreatedAt:  formatTime(snap.CreatedAt),
	}
}

func toProjectSnapshotDTOs(snaps []domain.ProjectSnapshot) []ProjectSnapshotDTO {
	result := make([]ProjectSnapshotDTO, 0, len(snaps))
	for i := range snaps {
		result = append(result, *toProjectSnapshotDTO(&snaps[i]))
	}
	return result
}

func toSynctexViewDTO(r *service.SynctexViewResult) *SynctexViewDTO {
	if r == nil {
		return nil
	}
	return &SynctexViewDTO{
		Page:   r.Page,
		X:      r.X,
		Y:      r.Y,
		H:      r.H,
		V:      r.V,
		W:      r.W,
		Height: r.Height,
	}
}

func toSynctexEditDTO(r *service.SynctexEditResult) *SynctexEditDTO {
	if r == nil {
		return nil
	}
	return &SynctexEditDTO{
		Input:  r.Input,
		Line:   r.Line,
		Column: r.Column,
		Offset: r.Offset,
	}
}

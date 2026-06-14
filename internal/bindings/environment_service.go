package bindings

import "github.com/kyangconn/goleaf/internal/service"

type LatexToolDTO struct {
	Name      string `json:"name"`
	Path      string `json:"path"`
	Version   string `json:"version"`
	Available bool   `json:"available"`
}

type LatexEnvironmentDTO struct {
	Compiler    string         `json:"compiler"`
	HasCompiler bool           `json:"has_compiler"`
	Tools       []LatexToolDTO `json:"tools"`
}

type TexLiveDownloadDTO struct {
	Variant       string   `json:"variant"`
	Scheme        string   `json:"scheme"`
	URL           string   `json:"url"`
	InstallerPath string   `json:"installer_path"`
	InstallArgs   []string `json:"install_args"`
	AlreadyExists bool     `json:"already_exists"`
	Size          int64    `json:"size"`
}

// EnvironmentService exposes local LaTeX environment setup operations.
type EnvironmentService struct {
	latex service.LatexEnvironmentService
}

func NewEnvironmentService(latex service.LatexEnvironmentService) *EnvironmentService {
	return &EnvironmentService{latex: latex}
}

func (s *EnvironmentService) CheckLatexEnvironment() (*LatexEnvironmentDTO, error) {
	status, err := s.latex.Check()
	if err != nil {
		return nil, err
	}
	return toLatexEnvironmentDTO(status), nil
}

func (s *EnvironmentService) DownloadTexLiveInstaller(variant string) (*TexLiveDownloadDTO, error) {
	result, err := s.latex.DownloadTexLiveInstaller(variant)
	if err != nil {
		return nil, err
	}
	return toTexLiveDownloadDTO(result), nil
}

func (s *EnvironmentService) ReloadLatexEnvironment() (*LatexEnvironmentDTO, error) {
	status, err := s.latex.ReloadEnvironment()
	if err != nil {
		return nil, err
	}
	return toLatexEnvironmentDTO(status), nil
}

func (s *EnvironmentService) StartTexLiveInstaller(variant string) (*TexLiveDownloadDTO, error) {
	result, err := s.latex.StartTexLiveInstaller(variant)
	if err != nil {
		return nil, err
	}
	return toTexLiveDownloadDTO(result), nil
}

func toLatexEnvironmentDTO(status *service.LatexEnvironmentStatus) *LatexEnvironmentDTO {
	if status == nil {
		return nil
	}
	tools := make([]LatexToolDTO, 0, len(status.Tools))
	for _, tool := range status.Tools {
		tools = append(tools, LatexToolDTO{
			Name:      tool.Name,
			Path:      tool.Path,
			Version:   tool.Version,
			Available: tool.Available,
		})
	}
	return &LatexEnvironmentDTO{
		Compiler:    status.Compiler,
		HasCompiler: status.HasCompiler,
		Tools:       tools,
	}
}

func toTexLiveDownloadDTO(result *service.TexLiveDownloadResult) *TexLiveDownloadDTO {
	if result == nil {
		return nil
	}
	return &TexLiveDownloadDTO{
		Variant:       result.Variant,
		Scheme:        result.Scheme,
		URL:           result.URL,
		InstallerPath: result.InstallerPath,
		InstallArgs:   result.InstallArgs,
		AlreadyExists: result.AlreadyExists,
		Size:          result.Size,
	}
}

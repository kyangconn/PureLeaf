package bindings

import "github.com/kyangconn/goleaf/internal/service"

// ProjectService exposes project operations to the Wails frontend.
type ProjectService struct {
	projects service.ProjectService
}

func NewProjectService(projects service.ProjectService) *ProjectService {
	return &ProjectService{projects: projects}
}

func (s *ProjectService) ListProjects() ([]ProjectDTO, error) {
	projects, err := s.projects.List()
	if err != nil {
		return nil, err
	}
	return toProjectDTOs(projects), nil
}

func (s *ProjectService) GetProject(id uint) (*ProjectDTO, error) {
	project, err := s.projects.GetByID(id)
	if err != nil {
		return nil, err
	}
	return toProjectDTO(project), nil
}

func (s *ProjectService) CreateProject(name string) (*ProjectDTO, error) {
	project, err := s.projects.Create(name)
	if err != nil {
		return nil, err
	}
	return toProjectDTO(project), nil
}

func (s *ProjectService) UpdateProject(id uint, name string) (*ProjectDTO, error) {
	project, err := s.projects.Update(id, name)
	if err != nil {
		return nil, err
	}
	return toProjectDTO(project), nil
}

func (s *ProjectService) DeleteProject(id uint) error {
	return s.projects.Delete(id)
}

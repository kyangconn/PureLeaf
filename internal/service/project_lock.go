package service

import "sync"

// ProjectLockManager serializes operations that mutate or consume one project.
type ProjectLockManager interface {
	WithProjectLock(projectID uint, fn func() error) error
}

type projectLockManager struct {
	mu    sync.Mutex
	locks map[uint]*sync.Mutex
}

// NewProjectLockManager creates an app-scoped lock registry.
func NewProjectLockManager() ProjectLockManager {
	return &projectLockManager{locks: make(map[uint]*sync.Mutex)}
}

func (m *projectLockManager) WithProjectLock(projectID uint, fn func() error) error {
	lock := m.lockFor(projectID)
	lock.Lock()
	defer lock.Unlock()
	return fn()
}

func (m *projectLockManager) lockFor(projectID uint) *sync.Mutex {
	m.mu.Lock()
	defer m.mu.Unlock()

	lock, ok := m.locks[projectID]
	if !ok {
		lock = &sync.Mutex{}
		m.locks[projectID] = lock
	}
	return lock
}

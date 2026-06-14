//go:build !windows

package service

func reloadProcessEnvironment() error {
	return nil
}

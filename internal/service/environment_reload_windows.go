//go:build windows

package service

import (
	"fmt"
	"os"
	"strings"

	"golang.org/x/sys/windows/registry"
)

const machineEnvironmentKey = `SYSTEM\CurrentControlSet\Control\Session Manager\Environment`

type windowsEnvironmentValue struct {
	name  string
	typ   uint32
	value string
}

func reloadProcessEnvironment() error {
	machineValues, machineErr := readWindowsEnvironment(registry.LOCAL_MACHINE, machineEnvironmentKey)
	userValues, userErr := readWindowsEnvironment(registry.CURRENT_USER, `Environment`)
	if machineErr != nil && userErr != nil {
		return fmt.Errorf("读取 Windows 环境变量失败: machine=%v user=%v", machineErr, userErr)
	}

	applyWindowsEnvironment(machineValues, false)
	applyWindowsEnvironment(userValues, false)
	reloadWindowsPath(machineValues, userValues)
	return nil
}

func readWindowsEnvironment(root registry.Key, path string) (map[string]windowsEnvironmentValue, error) {
	key, err := registry.OpenKey(root, path, registry.QUERY_VALUE)
	if err != nil {
		return nil, err
	}
	defer key.Close()

	names, err := key.ReadValueNames(0)
	if err != nil {
		return nil, err
	}

	values := make(map[string]windowsEnvironmentValue, len(names))
	for _, name := range names {
		value, typ, err := key.GetStringValue(name)
		if err != nil {
			continue
		}
		values[strings.ToLower(name)] = windowsEnvironmentValue{
			name:  name,
			typ:   typ,
			value: value,
		}
	}
	return values, nil
}

func applyWindowsEnvironment(values map[string]windowsEnvironmentValue, includePath bool) {
	for key, value := range values {
		if !includePath && key == "path" {
			continue
		}
		_ = os.Setenv(value.name, expandWindowsEnvironmentValue(value))
	}
}

func reloadWindowsPath(machineValues, userValues map[string]windowsEnvironmentValue) {
	machinePath := expandWindowsEnvironmentValue(machineValues["path"])
	if machinePath != "" {
		_ = os.Setenv(currentPathKey(), machinePath)
	}

	userRaw := userValues["path"].value
	userPath := expandWindowsEnvironmentValue(userValues["path"])
	finalPath := joinWindowsPath(machinePath, userPath, strings.Contains(strings.ToLower(userRaw), "%path%"))
	if finalPath == "" {
		return
	}
	_ = os.Setenv(currentPathKey(), finalPath)
}

func expandWindowsEnvironmentValue(value windowsEnvironmentValue) string {
	if value.typ != registry.EXPAND_SZ {
		return value.value
	}
	expanded, err := registry.ExpandString(value.value)
	if err != nil {
		return value.value
	}
	return expanded
}

func joinWindowsPath(machinePath, userPath string, userPathExpandsPath bool) string {
	switch {
	case userPathExpandsPath:
		return userPath
	case machinePath == "":
		return userPath
	case userPath == "":
		return machinePath
	default:
		return machinePath + ";" + userPath
	}
}

func currentPathKey() string {
	for _, item := range os.Environ() {
		key, _, ok := strings.Cut(item, "=")
		if ok && strings.EqualFold(key, "path") {
			return key
		}
	}
	return "Path"
}

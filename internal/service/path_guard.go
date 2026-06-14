package service

import (
	"fmt"
	"path/filepath"
	"strings"
	"unicode"

	"github.com/kyangconn/goleaf/internal/domain"
)

// 路径安全相关的校验。
//
// 防御目标：
//   - 禁止路径穿越（../、绝对路径、盘符前缀、UNC 路径）
//   - 禁止路径分隔符出现在文件名中（name 是单层名字，不是路径）
//   - 禁止 Windows 非法字符，保证项目跨平台移动不会因文件名损坏
//
// 这些校验放在 service 层，作为业务边界。即便未来新增调用点，
// 也应统一走这里，而不是各自实现。

var windowsReservedNames = map[string]struct{}{
	"CON": {}, "PRN": {}, "AUX": {}, "NUL": {},
	"COM1": {}, "COM2": {}, "COM3": {}, "COM4": {}, "COM5": {},
	"COM6": {}, "COM7": {}, "COM8": {}, "COM9": {},
	"LPT1": {}, "LPT2": {}, "LPT3": {}, "LPT4": {}, "LPT5": {},
	"LPT6": {}, "LPT7": {}, "LPT8": {}, "LPT9": {},
}

// ValidateFileName 校验单个文件/文件夹名字。
func ValidateFileName(name string) error {
	if name == "" {
		return fmt.Errorf("名称不能为空")
	}
	// 不做 TrimSpace：文件名首尾的空格在文件系统里是有意义的，
	// 直接拒绝比静默裁剪更安全（避免与预期不一致）。
	if strings.TrimSpace(name) == "" {
		return fmt.Errorf("名称不能全是空白字符")
	}
	if name == "." || name == ".." {
		return fmt.Errorf("名称不能是 . 或 ..")
	}

	// 路径分隔符：name 是单层，不应含任何分隔符。
	if strings.ContainsAny(name, `/\`) {
		return fmt.Errorf("名称不能包含路径分隔符")
	}

	// Windows 绝对路径前缀（盘符 / UNC）。
	if isWindowsAbsolutePath(name) {
		return fmt.Errorf("名称不能是绝对路径")
	}

	// Windows 文件系统非法字符（统一拦截，保证项目可跨平台移动）。
	if i := strings.IndexAny(name, `<>:"|?*`); i >= 0 {
		return fmt.Errorf("名称包含非法字符 %q", name[i])
	}

	// 控制字符。
	if i := strings.IndexFunc(name, unicode.IsControl); i >= 0 {
		return fmt.Errorf("名称包含控制字符")
	}

	// 末尾点或空格（Windows 会自动剥除，导致与预期不一致）。
	if name != strings.TrimRight(name, ". ") {
		return fmt.Errorf("名称不能以点或空格结尾")
	}

	// Windows 保留设备名（不区分大小写）。
	if _, ok := windowsReservedNames[strings.ToUpper(name)]; ok {
		return fmt.Errorf("名称是系统保留名")
	}

	// 长度上限（兼顾常见文件系统）。
	if len(name) > 255 {
		return fmt.Errorf("名称过长（最多 255 字节）")
	}
	return nil
}

// isWindowsAbsolutePath 检测是否是 Windows 风格的绝对路径前缀。
//   - 盘符: "C:", "D:\" 等（单字母 + 冒号）
//   - UNC:  "\\" 开头
func isWindowsAbsolutePath(name string) bool {
	// UNC 或反斜杠绝对路径
	if strings.HasPrefix(name, `\\`) {
		return true
	}
	// 盘符前缀，如 "C:..."
	if len(name) >= 2 {
		c := name[0]
		if (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') {
			if name[1] == ':' {
				return true
			}
		}
	}
	return false
}

// ValidateRelativePath 校验 computePath 算出的相对路径不逃逸项目目录。
//
// 允许的形态：a/b/main.tex（仅含名字段，用平台分隔符或 / 连接）。
// 禁止：绝对路径、.. 段、盘符前缀、UNC 前缀、以分隔符开头。
func ValidateRelativePath(rel string) error {
	if rel == "" {
		return fmt.Errorf("相对路径不能为空")
	}
	// 以任何分隔符开头都视为绝对路径（Windows 上 filepath.IsAbs 对 "/a" 返回 false，需补充检查）。
	if rel[0] == '/' || rel[0] == '\\' {
		return fmt.Errorf("相对路径不能是绝对路径")
	}
	if filepath.IsAbs(rel) {
		return fmt.Errorf("相对路径不能是绝对路径")
	}
	if isWindowsAbsolutePath(rel) {
		return fmt.Errorf("相对路径不能是绝对路径")
	}
	// 逐段检查 .. 段（兼容 / 和 \ 分隔符）。
	segments := splitPathSegments(rel)
	for _, seg := range segments {
		if seg == ".." {
			return fmt.Errorf("相对路径不能包含 .. 段")
		}
	}
	return nil
}

// splitPathSegments 兼容 / 和 \ 切分路径段。
func splitPathSegments(p string) []string {
	// 把反斜杠统一成正斜杠再切，避免 Windows 路径被当成单段。
	normalized := strings.ReplaceAll(p, `\`, `/`)
	parts := strings.Split(normalized, `/`)
	out := make([]string, 0, len(parts))
	for _, part := range parts {
		if part != "" {
			out = append(out, part)
		}
	}
	return out
}

// computeFilePath 根据 fileMap（id -> file）从某个 fileID 向上拼接出相对路径。
// 调用方负责保证 fileMap 已填充项目内所有文件。
func computeFilePath(fileMap map[uint]*domain.File, fileID uint) string {
	var parts []string
	currentID := fileID
	seen := make(map[uint]struct{}) // 防止循环引用死循环
	for {
		if _, loop := seen[currentID]; loop {
			break
		}
		seen[currentID] = struct{}{}
		f, ok := fileMap[currentID]
		if !ok {
			break
		}
		parts = append([]string{f.Name}, parts...)
		if f.ParentID == nil {
			break
		}
		currentID = *f.ParentID
	}
	return filepath.Join(parts...)
}

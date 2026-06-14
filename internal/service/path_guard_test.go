package service

import (
	"strings"
	"testing"

	"github.com/kyangconn/goleaf/internal/domain"
)

func TestValidateFileName(t *testing.T) {
	cases := []struct {
		name string
		ok   bool
	}{
		{"main.tex", true},
		{"chapter-1.tex", true},
		{"图1.png", true},
		{".latexmkrc", true}, // 隐藏文件允许
		{"", false},
		{"   ", false},
		{".", false},
		{"..", false},
		{"a/b", false},
		{`a\b`, false},
		{"../escape", false},
		{"C:evil", false},         // 盘符前缀
		{`\\server\share`, false}, // UNC
		{"a<b", false},
		{`a"b`, false},
		{"a|b", false},
		{"CON", false}, // Windows 保留名
		{"com1", false},
		{"trailing.", false},
		{"trailing ", false},
		{"ctrl\x00char", false},
		{strings.Repeat("a", 256), false}, // 过长
	}
	for _, c := range cases {
		err := ValidateFileName(c.name)
		if c.ok && err != nil {
			t.Errorf("期望 %q 通过，得到错误: %v", c.name, err)
		}
		if !c.ok && err == nil {
			t.Errorf("期望 %q 被拒绝，但通过了", c.name)
		}
	}
}

func TestValidateRelativePath(t *testing.T) {
	cases := []struct {
		path string
		ok   bool
	}{
		{"main.tex", true},
		{"chapters/intro.tex", true},
		{"a/b/c.tex", true},
		{"", false},
		{"/etc/passwd", false}, // Unix 绝对路径
		{"/a/b", false},
		{"../escape", false}, // .. 段
		{"a/../b", false},    // 中间 .. 段
		{"a/../../c", false},
		{`a\..\b`, false},         // 反斜杠 .. 段
		{"C:/Users", false},       // Windows 盘符
		{`\\server\share`, false}, // UNC
	}
	for _, c := range cases {
		err := ValidateRelativePath(c.path)
		if c.ok && err != nil {
			t.Errorf("期望 %q 通过，得到错误: %v", c.path, err)
		}
		if !c.ok && err == nil {
			t.Errorf("期望 %q 被拒绝，但通过了", c.path)
		}
	}
}

func TestComputeFilePath_NoLoop(t *testing.T) {
	// 循环引用不应死循环
	a := uint(1)
	b := uint(2)
	fileMap := map[uint]*domain.File{
		a: {ID: a, Name: "a", ParentID: &b},
		b: {ID: b, Name: "b", ParentID: &a},
	}
	_ = computeFilePath(fileMap, a) // 不死循环即通过
}

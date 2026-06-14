package service

import (
	"strings"
	"testing"
)

func TestComputeDiff_NoChange(t *testing.T) {
	text := "line1\nline2\nline3"
	got := ComputeDiff("a", "b", text, text)
	if got != "" {
		t.Errorf("无变化时应返回空串，得到:\n%s", got)
	}
}

func TestComputeDiff_Insertion(t *testing.T) {
	old := "line1\nline3"
	new := "line1\nline2\nline3"
	got := ComputeDiff("old", "new", old, new)
	if !strings.Contains(got, "+line2") {
		t.Errorf("diff 应包含 +line2，得到:\n%s", got)
	}
	if !strings.HasPrefix(got, "--- old") {
		t.Errorf("diff 应以 --- old 开头，得到:\n%s", got)
	}
}

func TestComputeDiff_Deletion(t *testing.T) {
	old := "line1\nline2\nline3"
	new := "line1\nline3"
	got := ComputeDiff("old", "new", old, new)
	if !strings.Contains(got, "-line2") {
		t.Errorf("diff 应包含 -line2，得到:\n%s", got)
	}
}

func TestComputeDiff_Modification(t *testing.T) {
	old := "keep\nchange me\nkeep"
	new := "keep\nchanged\nkeep"
	got := ComputeDiff("old", "new", old, new)
	// 修改应表现为删除旧行 + 插入新行
	if !strings.Contains(got, "-change me") || !strings.Contains(got, "+changed") {
		t.Errorf("diff 应体现修改，得到:\n%s", got)
	}
}

func TestComputeDiff_ContainsContextLines(t *testing.T) {
	old := "c1\nc2\nc3\nc4\nc5\nc6\nc7\nchanged\nc9\nc10\nc11"
	new := "c1\nc2\nc3\nc4\nc5\nc6\nc7\nCHANGED\nc9\nc10\nc11"
	got := ComputeDiff("old", "new", old, new)
	// 上下文行 c5/c6/c7 和 c9/c10/c11 应出现为 " " 前缀
	lines := strings.Split(got, "\n")
	contextCount := 0
	for _, l := range lines {
		if strings.HasPrefix(l, " ") {
			contextCount++
		}
	}
	if contextCount < 6 {
		t.Errorf("上下文行应 ≥6（前后各3），得到 %d:\n%s", contextCount, got)
	}
}

func TestComputeDiff_EmptyInputs(t *testing.T) {
	got := ComputeDiff("a", "b", "", "")
	if got != "" {
		t.Errorf("空输入应无 diff，得到 %q", got)
	}
}

func TestComputeDiff_AddToEmpty(t *testing.T) {
	got := ComputeDiff("a", "b", "", "new content")
	if !strings.Contains(got, "+new content") {
		t.Errorf("应包含新增行，得到:\n%s", got)
	}
}

func TestComputeDiff_HasHunkHeader(t *testing.T) {
	old := "a\nb\nc"
	new := "a\nB\nc"
	got := ComputeDiff("old", "new", old, new)
	if !strings.Contains(got, "@@") {
		t.Errorf("应包含 hunk 头 @@，得到:\n%s", got)
	}
}

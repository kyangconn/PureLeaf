package service

import "testing"

func TestParseSynctexView(t *testing.T) {
	// 来自真实 synctex view 输出
	output := `This is SyncTeX command line utility, version 1.5
SyncTeX result begin
Output:test_sync.pdf
Page:1
x:171.128296
y:134.764618
h:133.768356
v:134.764618
W:343.711060
H:6.918498
before:
offset:-1
middle:
after:
SyncTeX result end
`
	result, err := parseSynctexView(output)
	if err != nil {
		t.Fatalf("解析失败: %v", err)
	}
	if result.Page != 1 {
		t.Errorf("Page = %d, 期望 1", result.Page)
	}
	if result.X != 171.128296 {
		t.Errorf("X = %g, 期望 171.128296", result.X)
	}
	if result.Y != 134.764618 {
		t.Errorf("Y = %g, 期望 134.764618", result.Y)
	}
	if result.H != 133.768356 {
		t.Errorf("H = %g, 期望 133.768356", result.H)
	}
	if result.V != 134.764618 {
		t.Errorf("V = %g, 期望 134.764618", result.V)
	}
	if result.W != 343.711060 {
		t.Errorf("W = %g, 期望 343.711060", result.W)
	}
	if result.Height != 6.918498 {
		t.Errorf("Height = %g, 期望 6.918498", result.Height)
	}
}

func TestParseSynctexView_NoResultBlock(t *testing.T) {
	_, err := parseSynctexView("no result here")
	if err == nil {
		t.Errorf("应返回错误（无结果块）")
	}
}

func TestParseSynctexView_NoPage(t *testing.T) {
	output := `SyncTeX result begin
Output:test.pdf
Page:0
x:0
SyncTeX result end
`
	_, err := parseSynctexView(output)
	if err == nil {
		t.Errorf("Page=0 应返回错误")
	}
}

func TestParseSynctexEdit(t *testing.T) {
	// 来自真实 synctex edit 输出
	output := `This is SyncTeX command line utility, version 1.5
SyncTeX result begin
Output:test_sync.pdf
Input:C:/Users/test/./test_sync.tex
Line:7
Column:-1
Offset:0
Context:
SyncTeX result end
`
	result, err := parseSynctexEdit(output)
	if err != nil {
		t.Fatalf("解析失败: %v", err)
	}
	if result.Input != "C:/Users/test/./test_sync.tex" {
		t.Errorf("Input = %q", result.Input)
	}
	if result.Line != 7 {
		t.Errorf("Line = %d, 期望 7", result.Line)
	}
	if result.Column != -1 {
		t.Errorf("Column = %d, 期望 -1", result.Column)
	}
}

func TestParseSynctexEdit_NoLine(t *testing.T) {
	output := `SyncTeX result begin
Input:test.tex
Line:0
SyncTeX result end
`
	_, err := parseSynctexEdit(output)
	if err == nil {
		t.Errorf("Line=0 应返回错误")
	}
}

func TestSynctexFile(t *testing.T) {
	cases := []struct {
		pdf  string
		want string
	}{
		{"main.pdf", "main.synctex.gz"},
		{"/abs/path/main.pdf", "/abs/path/main.synctex.gz"},
		{"output", "output.synctex.gz"},
		{"a.b.pdf", "a.b.synctex.gz"},
	}
	for _, c := range cases {
		got := SynctexFile(c.pdf)
		if got != c.want {
			t.Errorf("SynctexFile(%q) = %q, 期望 %q", c.pdf, got, c.want)
		}
	}
}

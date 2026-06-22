package service

import (
	"bytes"
	"context"
	"fmt"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

// SynctexViewResult 是正向同步（源码 → PDF）的查询结果。
// Page 为 1-based 页码，x/y/h/v/W/H 单位为 big point (72 dpi)，
// 直接对应 synctex CLI 输出。
type SynctexViewResult struct {
	Page   int     `json:"page"`
	X      float64 `json:"x"`
	Y      float64 `json:"y"`
	H      float64 `json:"h"`
	V      float64 `json:"v"`
	W      float64 `json:"w"`
	Height float64 `json:"h_height"` // synctex 输出 H 字段（矩形高度），重命名避免与 h 混淆
}

// SynctexEditResult 是反向同步（PDF → 源码）的查询结果。
type SynctexEditResult struct {
	Input  string `json:"input"`
	Line   int    `json:"line"`
	Column int    `json:"column"`
	Offset int    `json:"offset"`
}

// synctex 调用 synctex CLI 并返回原始输出。
// workDir 为项目目录（synctex 文件所在），args 为 CLI 参数。
func synctex(workDir string, args ...string) (string, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, "synctex", args...)
	cmd.Dir = workDir
	var stdout, stderr bytes.Buffer
	cmd.Stdout = &stdout
	cmd.Stderr = &stderr

	if err := cmd.Run(); err != nil {
		if ctx.Err() == context.DeadlineExceeded {
			return "", fmt.Errorf("synctex 查询超时")
		}
		stderrText := strings.TrimSpace(stderr.String())
		if stderrText != "" {
			return "", fmt.Errorf("synctex 失败: %s", stderrText)
		}
		return "", fmt.Errorf("synctex 执行失败: %w", err)
	}
	return stdout.String(), nil
}

// SynctexView 正向同步：根据源码位置查询 PDF 中对应区域。
// input 为 TeX 源文件名（TeX 认识的名字，通常是相对项目目录的路径），
// line/column 为 1-based，pdfPath 为输出 PDF 路径（相对 workDir 或绝对）。
func SynctexView(workDir, input string, line, column int, pdfPath string) (*SynctexViewResult, error) {
	if line < 1 {
		line = 1
	}
	if column < 0 {
		column = 0
	}
	// -i line:column:[page_hint:]input，page_hint 传 0（无提示）
	inputSpec := fmt.Sprintf("%d:%d:0:%s", line, column, input)
	output, err := synctex(workDir, "view", "-i", inputSpec, "-o", pdfPath, "-d", workDir)
	if err != nil {
		return nil, err
	}
	return parseSynctexView(output)
}

// SynctexEdit 反向同步：根据 PDF 页面坐标查询源码位置。
// page 为 1-based，x/y 为页面内坐标（big point, 72 dpi），从页面左上角算起。
func SynctexEdit(workDir string, page int, x, y float64, pdfPath string) (*SynctexEditResult, error) {
	if page < 1 {
		page = 1
	}
	outputSpec := fmt.Sprintf("%d:%g:%g:%s", page, x, y, pdfPath)
	output, err := synctex(workDir, "edit", "-o", outputSpec, "-d", workDir)
	if err != nil {
		return nil, err
	}
	return parseSynctexEdit(output)
}

// parseSynctexView 解析 `synctex view` 的 "SyncTeX result begin...end" 块。
func parseSynctexView(output string) (*SynctexViewResult, error) {
	result := &SynctexViewResult{}
	found := false
	for _, line := range strings.Split(output, "\n") {
		line = strings.TrimSpace(line)
		if line == "SyncTeX result begin" {
			found = true
			continue
		}
		if line == "SyncTeX result end" {
			break
		}
		if !found {
			continue
		}
		// 行格式: Key:Value
		idx := strings.Index(line, ":")
		if idx < 0 {
			continue
		}
		key := line[:idx]
		val := strings.TrimSpace(line[idx+1:])
		switch key {
		case "Page":
			if n, err := strconv.Atoi(val); err == nil {
				result.Page = n
			}
		case "x":
			result.X = parseFloat(val)
		case "y":
			result.Y = parseFloat(val)
		case "h":
			result.H = parseFloat(val)
		case "v":
			result.V = parseFloat(val)
		case "W":
			result.W = parseFloat(val)
		case "H":
			result.Height = parseFloat(val)
		}
	}
	if !found {
		return nil, fmt.Errorf("synctex 输出无结果块")
	}
	if result.Page == 0 {
		return nil, fmt.Errorf("synctex 未找到对应 PDF 页面")
	}
	return result, nil
}

// parseSynctexEdit 解析 `synctex edit` 的结果块。
func parseSynctexEdit(output string) (*SynctexEditResult, error) {
	result := &SynctexEditResult{}
	found := false
	for _, line := range strings.Split(output, "\n") {
		line = strings.TrimSpace(line)
		if line == "SyncTeX result begin" {
			found = true
			continue
		}
		if line == "SyncTeX result end" {
			break
		}
		if !found {
			continue
		}
		idx := strings.Index(line, ":")
		if idx < 0 {
			continue
		}
		key := line[:idx]
		val := strings.TrimSpace(line[idx+1:])
		switch key {
		case "Input":
			result.Input = val
		case "Line":
			if n, err := strconv.Atoi(val); err == nil {
				result.Line = n
			}
		case "Column":
			if n, err := strconv.Atoi(val); err == nil {
				result.Column = n
			}
		case "Offset":
			if n, err := strconv.Atoi(val); err == nil {
				result.Offset = n
			}
		}
	}
	if !found {
		return nil, fmt.Errorf("synctex 输出无结果块")
	}
	if result.Line == 0 {
		return nil, fmt.Errorf("synctex 未找到对应源码行")
	}
	return result, nil
}

// parseFloat 解析浮点，失败返回 0（synctex 坐标缺失时用 0 兜底）。
func parseFloat(s string) float64 {
	f, _ := strconv.ParseFloat(s, 64)
	return f
}

// HasSynctex 检查 synctex CLI 是否可用。
func HasSynctex() bool {
	_, err := exec.LookPath("synctex")
	return err == nil
}

// SynctexFile 拼接 synctex 文件名（输出同名 + .synctex.gz）。
// 用于判断某次编译是否生成了 synctex 数据。
func SynctexFile(pdfPath string) string {
	ext := filepath.Ext(pdfPath)
	if ext == "" {
		return pdfPath + ".synctex.gz"
	}
	return strings.TrimSuffix(pdfPath, ext) + ".synctex.gz"
}

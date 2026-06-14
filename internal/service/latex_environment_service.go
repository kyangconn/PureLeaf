package service

import (
	"bufio"
	"context"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"time"
)

const (
	texLiveWindowsInstallerURL = "https://mirror.ctan.org/systems/texlive/tlnet/install-tl-windows.exe"
	texLiveUnixInstallerURL    = "https://mirror.ctan.org/systems/texlive/tlnet/install-tl-unx.tar.gz"
)

type LatexEnvironmentService interface {
	Check() (*LatexEnvironmentStatus, error)
	DownloadTexLiveInstaller(variant string) (*TexLiveDownloadResult, error)
	ReloadEnvironment() (*LatexEnvironmentStatus, error)
	StartTexLiveInstaller(variant string) (*TexLiveDownloadResult, error)
}

type LatexToolStatus struct {
	Name      string `json:"name"`
	Path      string `json:"path"`
	Version   string `json:"version"`
	Available bool   `json:"available"`
}

type LatexEnvironmentStatus struct {
	Compiler    string            `json:"compiler"`
	HasCompiler bool              `json:"has_compiler"`
	Tools       []LatexToolStatus `json:"tools"`
}

type TexLiveDownloadResult struct {
	Variant       string   `json:"variant"`
	Scheme        string   `json:"scheme"`
	URL           string   `json:"url"`
	InstallerPath string   `json:"installer_path"`
	InstallArgs   []string `json:"install_args"`
	AlreadyExists bool     `json:"already_exists"`
	Size          int64    `json:"size"`
}

type latexEnvironmentService struct {
	compiler    string
	downloadDir string
}

func NewLatexEnvironmentService(compiler, downloadDir string) LatexEnvironmentService {
	return &latexEnvironmentService{compiler: compiler, downloadDir: downloadDir}
}

func (s *latexEnvironmentService) Check() (*LatexEnvironmentStatus, error) {
	toolNames := uniqueTools([]string{s.compiler, "pdflatex", "xelatex", "lualatex", "pdftex", "tlmgr"})
	tools := make([]LatexToolStatus, 0, len(toolNames))
	hasCompiler := false

	for _, name := range toolNames {
		status := inspectLatexTool(name)
		if sameTool(name, s.compiler) && status.Available {
			hasCompiler = true
		}
		tools = append(tools, status)
	}

	return &LatexEnvironmentStatus{
		Compiler:    s.compiler,
		HasCompiler: hasCompiler,
		Tools:       tools,
	}, nil
}

func (s *latexEnvironmentService) ReloadEnvironment() (*LatexEnvironmentStatus, error) {
	if err := reloadProcessEnvironment(); err != nil {
		return nil, err
	}
	return s.Check()
}

func (s *latexEnvironmentService) DownloadTexLiveInstaller(variant string) (*TexLiveDownloadResult, error) {
	plan, err := s.texLivePlan(variant)
	if err != nil {
		return nil, err
	}
	if err := os.MkdirAll(s.downloadDir, 0755); err != nil {
		return nil, fmt.Errorf("创建下载目录失败: %w", err)
	}

	if info, err := os.Stat(plan.InstallerPath); err == nil && info.Size() > 0 {
		plan.AlreadyExists = true
		plan.Size = info.Size()
		return plan, nil
	}

	ctx, cancel := context.WithTimeout(context.Background(), 30*time.Minute)
	defer cancel()

	req, err := http.NewRequestWithContext(ctx, http.MethodGet, plan.URL, nil)
	if err != nil {
		return nil, err
	}
	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return nil, fmt.Errorf("下载 TeX Live 安装器失败: %w", err)
	}
	defer resp.Body.Close()
	if resp.StatusCode < http.StatusOK || resp.StatusCode >= http.StatusMultipleChoices {
		return nil, fmt.Errorf("下载 TeX Live 安装器失败: %s", resp.Status)
	}

	tmpPath := plan.InstallerPath + ".download"
	out, err := os.Create(tmpPath)
	if err != nil {
		return nil, fmt.Errorf("创建临时下载文件失败: %w", err)
	}
	size, copyErr := io.Copy(out, resp.Body)
	closeErr := out.Close()
	if copyErr != nil {
		_ = os.Remove(tmpPath)
		return nil, fmt.Errorf("写入 TeX Live 安装器失败: %w", copyErr)
	}
	if closeErr != nil {
		_ = os.Remove(tmpPath)
		return nil, fmt.Errorf("关闭 TeX Live 安装器文件失败: %w", closeErr)
	}
	if err := os.Rename(tmpPath, plan.InstallerPath); err != nil {
		_ = os.Remove(tmpPath)
		return nil, fmt.Errorf("保存 TeX Live 安装器失败: %w", err)
	}

	plan.Size = size
	return plan, nil
}

func (s *latexEnvironmentService) StartTexLiveInstaller(variant string) (*TexLiveDownloadResult, error) {
	plan, err := s.DownloadTexLiveInstaller(variant)
	if err != nil {
		return nil, err
	}

	var cmd *exec.Cmd
	switch runtime.GOOS {
	case "windows":
		cmd = exec.Command(plan.InstallerPath, plan.InstallArgs...)
	case "darwin":
		cmd = exec.Command("open", plan.InstallerPath)
	default:
		cmd = exec.Command("xdg-open", plan.InstallerPath)
	}
	if err := cmd.Start(); err != nil {
		return nil, fmt.Errorf("启动 TeX Live 安装器失败: %w", err)
	}
	return plan, nil
}

func (s *latexEnvironmentService) texLivePlan(variant string) (*TexLiveDownloadResult, error) {
	normalized, scheme, err := normalizeTexLiveVariant(variant)
	if err != nil {
		return nil, err
	}

	installerName := "install-tl-unx.tar.gz"
	url := texLiveUnixInstallerURL
	if runtime.GOOS == "windows" {
		installerName = "install-tl-windows.exe"
		url = texLiveWindowsInstallerURL
	}

	return &TexLiveDownloadResult{
		Variant:       normalized,
		Scheme:        scheme,
		URL:           url,
		InstallerPath: filepath.Join(s.downloadDir, installerName),
		InstallArgs:   []string{"--scheme=" + scheme},
	}, nil
}

func normalizeTexLiveVariant(variant string) (string, string, error) {
	switch strings.ToLower(strings.TrimSpace(variant)) {
	case "", "base", "basic":
		return "base", "basic", nil
	case "small":
		return "small", "small", nil
	case "medium":
		return "medium", "medium", nil
	case "full":
		return "full", "full", nil
	default:
		return "", "", fmt.Errorf("未知 TeX Live 安装类型: %s", variant)
	}
}

func uniqueTools(names []string) []string {
	seen := make(map[string]struct{}, len(names))
	result := make([]string, 0, len(names))
	for _, name := range names {
		name = strings.TrimSpace(name)
		if name == "" {
			continue
		}
		key := strings.ToLower(filepath.Base(name))
		if _, ok := seen[key]; ok {
			continue
		}
		seen[key] = struct{}{}
		result = append(result, name)
	}
	return result
}

func sameTool(a, b string) bool {
	return strings.EqualFold(filepath.Base(strings.TrimSpace(a)), filepath.Base(strings.TrimSpace(b)))
}

func inspectLatexTool(name string) LatexToolStatus {
	status := LatexToolStatus{Name: filepath.Base(name)}
	path, err := exec.LookPath(name)
	if err != nil {
		return status
	}
	status.Available = true
	status.Path = path
	status.Version = readToolVersion(path)
	return status
}

func readToolVersion(path string) string {
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()

	cmd := exec.CommandContext(ctx, path, "--version")
	output, err := cmd.Output()
	if err != nil {
		return ""
	}
	scanner := bufio.NewScanner(strings.NewReader(string(output)))
	if !scanner.Scan() {
		return ""
	}
	return strings.TrimSpace(scanner.Text())
}

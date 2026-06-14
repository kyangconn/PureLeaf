// Package config 提供应用配置的加载与管理。
// 配置来源优先级: 环境变量 > 配置文件 > 默认值
package config

import (
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"

	"github.com/spf13/viper"
)

// Config 聚合所有子配置模块
type Config struct {
	LogFile  string
	Database DatabaseConfig
	Latex    LatexConfig
}

// DatabaseConfig 数据库相关配置
type DatabaseConfig struct {
	Path string // SQLite 数据库文件路径
}

// LatexConfig LaTeX 编译相关配置
type LatexConfig struct {
	Compiler string // 编译器命令: pdflatex, xelatex, lualatex
	Timeout  int    // 编译超时秒数
}

// Load 从配置文件、环境变量加载配置，未设置项使用默认值
func Load(configPath string) (*Config, error) {
	v := viper.New()

	// ① 默认值（优先级最低）
	setDefaults(v)

	// ② 配置文件
	if configPath != "" {
		v.SetConfigFile(configPath)
	} else {
		v.SetConfigName("config")
		v.SetConfigType("yaml")
		v.AddConfigPath(".")
	}

	if err := v.ReadInConfig(); err != nil {
		if _, ok := err.(viper.ConfigFileNotFoundError); !ok {
			return nil, fmt.Errorf("读取配置文件失败: %w", err)
		}
		// 文件不存在继续，仅依赖默认值
	}

	// ③ 环境变量（优先级最高，覆盖文件值）
	v.SetEnvPrefix("GOLEAF")
	v.SetEnvKeyReplacer(strings.NewReplacer(".", "_"))
	v.AutomaticEnv()
	bindEnvs(v)

	// 直接逐项读取（避免 Unmarshal 的类型转换陷阱）
	cfg := &Config{
		LogFile: v.GetString("log_file"),
		Database: DatabaseConfig{
			Path: v.GetString("database.path"),
		},
		Latex: LatexConfig{
			Compiler: v.GetString("latex.compiler"),
			Timeout:  v.GetInt("latex.timeout"),
		},
	}
	if strings.TrimSpace(cfg.Database.Path) == "" {
		cfg.Database.Path = defaultDatabasePath()
	}

	// ④ 校验关键配置
	if err := cfg.Validate(); err != nil {
		return nil, err
	}

	return cfg, nil
}

// Validate 校验配置合法性
func (c *Config) Validate() error {
	if c.Latex.Timeout <= 0 {
		return fmt.Errorf("latex.timeout 必须大于 0")
	}
	return nil
}

// bindEnvs 将配置键与环境变量做显式绑定
func bindEnvs(v *viper.Viper) {
	_ = v.BindEnv("log_file", "GOLEAF_LOG_FILE")
	_ = v.BindEnv("database.path", "GOLEAF_DATABASE_PATH")
	_ = v.BindEnv("latex.compiler", "GOLEAF_LATEX_COMPILER")
	_ = v.BindEnv("latex.timeout", "GOLEAF_LATEX_TIMEOUT")
}

// setDefaults 设置所有配置项的默认值
func setDefaults(v *viper.Viper) {
	v.SetDefault("log_file", "")
	v.SetDefault("database.path", defaultDatabasePath())
	v.SetDefault("latex.compiler", "pdflatex")
	v.SetDefault("latex.timeout", 60)
}

// defaultDatabasePath 返回默认数据库路径，放在按平台约定的用户数据目录下。
//
// 平台规则:
//   - Linux:   $XDG_DATA_HOME/goleaf，未设置则 ~/.local/share/goleaf
//   - macOS:   ~/Library/Application Support/goleaf
//   - Windows: %LOCALAPPDATA%/goleaf，未设置则回退 %APPDATA%/goleaf
//
// 数据目录与配置目录语义不同：projects/、.backup/、goleaf.db 都属于用户数据，
// 不应进入 XDG_CONFIG_HOME / ~/.config。
func defaultDatabasePath() string {
	dir, err := userDataDir()
	if err != nil || dir == "" {
		return filepath.Join(".", "data", "goleaf.db")
	}
	return filepath.Join(dir, "goleaf.db")
}

// userDataDir 返回平台约定的用户数据根目录（不含应用子目录）。
func userDataDir() (string, error) {
	switch runtime.GOOS {
	case "windows":
		if dir := os.Getenv("LOCALAPPDATA"); dir != "" {
			return filepath.Join(dir, "goleaf"), nil
		}
		if dir := os.Getenv("APPDATA"); dir != "" {
			return filepath.Join(dir, "goleaf"), nil
		}
		return "", fmt.Errorf("无法确定 Windows 用户数据目录（LOCALAPPDATA / APPDATA 均未设置）")
	case "darwin":
		home, err := os.UserHomeDir()
		if err != nil || home == "" {
			return "", fmt.Errorf("无法确定 macOS 用户目录: %w", err)
		}
		return filepath.Join(home, "Library", "Application Support", "goleaf"), nil
	default:
		// Linux / *BSD / 其他 Unix：遵循 XDG Base Directory Specification
		if dir := os.Getenv("XDG_DATA_HOME"); dir != "" {
			if err := validateXdgDir(dir); err != nil {
				return "", err
			}
			return filepath.Join(dir, "goleaf"), nil
		}
		home, err := os.UserHomeDir()
		if err != nil || home == "" {
			return "", fmt.Errorf("无法确定用户目录: %w", err)
		}
		return filepath.Join(home, ".local", "share", "goleaf"), nil
	}
}

// validateXdgDir 校验 XDG_DATA_HOME 不是空串或单纯的 home。
func validateXdgDir(dir string) error {
	abs, err := filepath.Abs(dir)
	if err != nil {
		return fmt.Errorf("XDG_DATA_HOME 无法解析为绝对路径: %w", err)
	}
	if strings.TrimSpace(abs) == "" {
		return fmt.Errorf("XDG_DATA_HOME 不能为空")
	}
	return nil
}

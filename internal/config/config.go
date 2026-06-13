// Package config 提供应用配置的加载与管理。
// 配置来源优先级: 环境变量 > 配置文件 > 默认值
package config

import (
	"fmt"
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
	v.SetDefault("database.path", "./data/goleaf.db")
	v.SetDefault("latex.compiler", "pdflatex")
	v.SetDefault("latex.timeout", 60)
}

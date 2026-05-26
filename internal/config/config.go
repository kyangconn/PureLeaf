// Package config 提供应用配置的加载与管理。
// 配置来源优先级: 命令行参数 > 环境变量 > 配置文件 > 默认值
package config

import (
	"fmt"
	"strings"

	"github.com/spf13/viper"
)

// Config 聚合所有子配置模块
type Config struct {
	Server   ServerConfig
	Database DatabaseConfig
	JWT      JWTConfig
	Latex    LatexConfig
}

// ServerConfig 服务相关配置
type ServerConfig struct {
	Port int    // 监听端口
	Mode string // 运行模式: debug, release, test
}

// DatabaseConfig 数据库相关配置
type DatabaseConfig struct {
	Path string // SQLite 数据库文件路径
}

// JWTConfig JWT 认证相关配置
type JWTConfig struct {
	Secret     string // JWT 签名密钥
	ExpireHour int    // Token 过期小时数
}

// LatexConfig LaTeX 编译相关配置
type LatexConfig struct {
	Compiler string // 编译器命令: pdflatex, xelatex, lualatex
	Timeout  int    // 编译超时秒数
}

// Load 从配置文件和命令行参数加载配置
func Load(configPath string) (*Config, error) {
	v := viper.New()

	// --- 配置文件 ---
	if configPath != "" {
		v.SetConfigFile(configPath)
	} else {
		// 默认搜索路径
		v.SetConfigName("config")
		v.SetConfigType("yaml")
		v.AddConfigPath(".")
		v.AddConfigPath("./config")
	}

	if err := v.ReadInConfig(); err != nil {
		if _, ok := err.(viper.ConfigFileNotFoundError); !ok {
			return nil, fmt.Errorf("读取配置文件失败: %w", err)
		}
		// 配置文件不存在时仅使用默认值，不报错
	}

	// --- 环境变量绑定 (GOLEAF_ 前缀) ---
	v.SetEnvPrefix("GOLEAF")
	v.SetEnvKeyReplacer(strings.NewReplacer(".", "_"))
	v.AutomaticEnv()

	// 绑定具体键名，便于 viper 自动映射
	bindEnvs(v)

	// --- 默认值 ---
	setDefaults(v)

	// --- CLI 参数绑定 (通过 viper 的 pflag 支持) ---
	v.SetConfigName("config") // 让 viper 知道参数名
	// 注意: 命令行参数需要在使用处 (cmd/server/main.go) 与 pflag 集成

	var cfg Config
	if err := v.Unmarshal(&cfg); err != nil {
		return nil, fmt.Errorf("解析配置失败: %w", err)
	}

	return &cfg, nil
}

// bindEnvs 将配置键与环境变量做显式绑定
func bindEnvs(v *viper.Viper) {
	_ = v.BindEnv("server.port", "GOLEAF_SERVER_PORT")
	_ = v.BindEnv("server.mode", "GOLEAF_SERVER_MODE")
	_ = v.BindEnv("database.path", "GOLEAF_DATABASE_PATH")
	_ = v.BindEnv("jwt.secret", "GOLEAF_JWT_SECRET")
	_ = v.BindEnv("jwt.expire_hour", "GOLEAF_JWT_EXPIRE_HOUR")
	_ = v.BindEnv("latex.compiler", "GOLEAF_LATEX_COMPILER")
	_ = v.BindEnv("latex.timeout", "GOLEAF_LATEX_TIMEOUT")
}

// setDefaults 设置所有配置项的默认值
func setDefaults(v *viper.Viper) {
	v.SetDefault("server.port", 8080)
	v.SetDefault("server.mode", "debug")
	v.SetDefault("database.path", "./data/goleaf.db")
	v.SetDefault("jwt.secret", "change-me-in-production")
	v.SetDefault("jwt.expire_hour", 24)
	v.SetDefault("latex.compiler", "pdflatex")
	v.SetDefault("latex.timeout", 60)
}

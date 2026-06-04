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
	Server   ServerConfig
	Database DatabaseConfig
	JWT      JWTConfig
	Latex    LatexConfig
}

// ServerConfig 服务相关配置
type ServerConfig struct {
	Port    int    // 监听端口
	Mode    string // 运行模式: debug, release, test
	LogFile string // 日志文件路径 (空=仅 stdout)
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
		v.AddConfigPath("./config")
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
		Server: ServerConfig{
			Port:    v.GetInt("server.port"),
			Mode:    v.GetString("server.mode"),
			LogFile: v.GetString("server.log_file"),
		},
		Database: DatabaseConfig{
			Path: v.GetString("database.path"),
		},
		JWT: JWTConfig{
			Secret:     v.GetString("jwt.secret"),
			ExpireHour: v.GetInt("jwt.expire_hour"),
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
	if c.JWT.ExpireHour <= 0 {
		return fmt.Errorf("jwt.expire_hour 必须大于 0，当前值: %d (请检查环境变量 GOLEAF_JWT_EXPIRE_HOUR)", c.JWT.ExpireHour)
	}
	if c.JWT.Secret == "" {
		return fmt.Errorf("jwt.secret 不能为空")
	}
	if c.Latex.Timeout <= 0 {
		return fmt.Errorf("latex.timeout 必须大于 0")
	}
	if c.Server.Port <= 0 {
		return fmt.Errorf("server.port 必须大于 0")
	}
	return nil
}

// bindEnvs 将配置键与环境变量做显式绑定
func bindEnvs(v *viper.Viper) {
	_ = v.BindEnv("server.port", "GOLEAF_SERVER_PORT")
	_ = v.BindEnv("server.mode", "GOLEAF_SERVER_MODE")
	_ = v.BindEnv("server.log_file", "GOLEAF_SERVER_LOG_FILE")
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
	v.SetDefault("server.log_file", "")
	v.SetDefault("database.path", "./data/goleaf.db")
	v.SetDefault("jwt.secret", "change-me-in-production")
	v.SetDefault("jwt.expire_hour", 24)
	v.SetDefault("latex.compiler", "pdflatex")
	v.SetDefault("latex.timeout", 60)
}

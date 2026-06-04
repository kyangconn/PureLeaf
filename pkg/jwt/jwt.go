// Package jwt 封装 JWT 的生成与校验
package jwt

import (
	"fmt"
	"time"

	jwtlib "github.com/golang-jwt/jwt/v5"
)

// MapClaims 重新导出 jwt 标准库的 MapClaims，方便调用方使用
type MapClaims = jwtlib.MapClaims

// Manager JWT 管理器
type Manager struct {
	secret     string
	expireHour int
}

// NewManager 创建 JWT 管理器
func NewManager(secret string, expireHour int) *Manager {
	return &Manager{secret: secret, expireHour: expireHour}
}

// Generate 生成 JWT Token
func (m *Manager) Generate(userID uint) (string, error) {
	now := time.Now()
	claims := jwtlib.MapClaims{
		"user_id": userID,
		"exp":     now.Add(time.Duration(m.expireHour) * time.Hour).Unix(),
		"iat":     now.Unix(),
	}
	token := jwtlib.NewWithClaims(jwtlib.SigningMethodHS256, claims)
	return token.SignedString([]byte(m.secret))
}

// Validate 校验 JWT Token，返回解析后的 token
func (m *Manager) Validate(tokenString string) (*jwtlib.Token, error) {
	return jwtlib.Parse(tokenString, func(token *jwtlib.Token) (interface{}, error) {
		if _, ok := token.Method.(*jwtlib.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("非预期的签名算法: %v", token.Header["alg"])
		}
		return []byte(m.secret), nil
	})
}

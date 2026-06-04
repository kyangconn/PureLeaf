// Package middleware 提供 HTTP 中间件
package middleware

import (
	"log"
	"net/http"
	"strings"

	"github.com/gin-gonic/gin"
	"github.com/golang-jwt/jwt/v5"
)

// JWTAuth 返回 JWT 认证中间件
// 从 Authorization: Bearer <token> 中解析 JWT，并将 userID 注入上下文
func JWTAuth(secret string) gin.HandlerFunc {
	return func(c *gin.Context) {
		authHeader := c.GetHeader("Authorization")
		if authHeader == "" {
			log.Printf("[JWT] 缺少 Authorization 头")
			c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{"error": "未提供认证令牌"})
			return
		}

		parts := strings.SplitN(authHeader, " ", 2)
		if len(parts) != 2 || !strings.EqualFold(parts[0], "Bearer") {
			log.Printf("[JWT] Authorization 头格式错误: %s", authHeader)
			c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{"error": "认证格式错误"})
			return
		}

		tokenString := parts[1]
		log.Printf("[JWT] 收到 Token (前20字符): %s...", truncate(tokenString, 20))

		token, err := jwt.Parse(tokenString, func(token *jwt.Token) (interface{}, error) {
			// jwt v5: SigningMethodHS256 本身就是 *SigningMethodHMAC
			if _, ok := token.Method.(*jwt.SigningMethodHMAC); !ok {
				log.Printf("[JWT] 非预期的签名算法: %v", token.Header["alg"])
				return nil, jwt.ErrSignatureInvalid
			}
			return []byte(secret), nil
		})

		if err != nil {
			log.Printf("[JWT] Parse 失败: %v", err)
			c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{"error": "认证令牌解析失败: " + err.Error()})
			return
		}
		if !token.Valid {
			log.Printf("[JWT] Token 无效 (exp=%v)", token.Claims)
			c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{"error": "认证令牌无效 (签名/过期检查未通过)"})
			return
		}

		claims, ok := token.Claims.(jwt.MapClaims)
		if !ok {
			log.Printf("[JWT] Claims 类型断言失败, 实际类型=%T", token.Claims)
			c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{"error": "无法解析令牌声明"})
			return
		}

		userIDFloat, ok := claims["user_id"].(float64)
		if !ok {
			log.Printf("[JWT] user_id 断言失败, 实际值=%v 类型=%T", claims["user_id"], claims["user_id"])
			c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{"error": "令牌中缺少用户标识"})
			return
		}

		log.Printf("[JWT] 认证成功 userID=%d", uint(userIDFloat))
		c.Set("userID", uint(userIDFloat))
		c.Next()
	}
}

func truncate(s string, n int) string {
	if len(s) <= n {
		return s
	}
	return s[:n]
}

// CORS 跨域中间件 — 开发环境下允许前端 dev server 跨域请求
func CORS() gin.HandlerFunc {
	return func(c *gin.Context) {
		c.Header("Access-Control-Allow-Origin", "*")
		c.Header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		c.Header("Access-Control-Allow-Headers", "Content-Type, Authorization")

		if c.Request.Method == http.MethodOptions {
			c.AbortWithStatus(http.StatusNoContent)
			return
		}

		c.Next()
	}
}

package middleware

import (
	"strings"

	"github.com/gin-gonic/gin"

	jwtpkg "github.com/kyangconn/goleaf/pkg/jwt"
	pklog "github.com/kyangconn/goleaf/pkg/log"
	resp "github.com/kyangconn/goleaf/pkg/response"
)

// JWTAuth 返回 JWT 认证中间件
// 从 Authorization: Bearer <token> 中解析 JWT，并将 userID 注入上下文
func JWTAuth(jwtMgr *jwtpkg.Manager) gin.HandlerFunc {
	return func(c *gin.Context) {
		authHeader := c.GetHeader("Authorization")
		if authHeader == "" {
			resp.Unauthorized(c, "未提供认证令牌")
			return
		}

		parts := strings.SplitN(authHeader, " ", 2)
		if len(parts) != 2 || !strings.EqualFold(parts[0], "Bearer") {
			resp.Unauthorized(c, "认证格式错误")
			return
		}

		tokenString := parts[1]

		token, err := jwtMgr.Validate(tokenString)
		if err != nil {
			pklog.Infof("[JWT] 验证失败: %v", err)
			resp.Unauthorized(c, "认证令牌无效")
			return
		}

		claims, ok := token.Claims.(jwtpkg.MapClaims)
		if !ok {
			pklog.Infof("[JWT] Claims 类型断言失败, 实际类型=%T", token.Claims)
			resp.Unauthorized(c, "无法解析令牌声明")
			return
		}

		userIDFloat, ok := claims["user_id"].(float64)
		if !ok {
			pklog.Infof("[JWT] user_id 断言失败, 实际值=%v 类型=%T", claims["user_id"], claims["user_id"])
			resp.Unauthorized(c, "令牌中缺少用户标识")
			return
		}

		pklog.Infof("[JWT] 认证成功 userID=%d", uint(userIDFloat))
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

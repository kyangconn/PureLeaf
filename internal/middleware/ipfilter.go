package middleware

import (
	"net"
	"strings"

	"github.com/gin-gonic/gin"

	pklog "github.com/kyangconn/goleaf/pkg/log"
	resp "github.com/kyangconn/goleaf/pkg/response"
)

// SetupIPFilter 限制 setup 相关接口仅允许受信任的 IP 访问。
// 通过 X-Forwarded-For / X-Real-IP 识别反代后的真实客户端 IP，
// 只有 localhost 地址和配置的 trustedProxies 中的 IP 才能通过。
func SetupIPFilter(trustedProxies []string) gin.HandlerFunc {
	// 构建可信 IP 集合（含所有 localhost 变体）
	trusted := map[string]bool{
		"127.0.0.1": true,
		"::1":       true,
		"localhost": true,
	}
	for _, ip := range trustedProxies {
		trusted[strings.TrimSpace(ip)] = true
	}

	return func(c *gin.Context) {
		clientIP := extractClientIP(c)
		if !isTrustedIP(clientIP, trusted) {
			pklog.Infof("[IPFilter] 拒绝非可信 IP 访问 setup: %s", clientIP)
			resp.Forbidden(c, "初始化设置仅允许从受信任的地址访问")
			return
		}
		c.Next()
	}
}

// extractClientIP 按优先级提取真实客户端 IP
func extractClientIP(c *gin.Context) string {
	// 1. X-Forwarded-For（取第一个，即原始客户端）
	if xff := c.GetHeader("X-Forwarded-For"); xff != "" {
		if idx := strings.IndexByte(xff, ','); idx > 0 {
			return strings.TrimSpace(xff[:idx])
		}
		return strings.TrimSpace(xff)
	}
	// 2. X-Real-IP
	if xri := c.GetHeader("X-Real-IP"); xri != "" {
		return strings.TrimSpace(xri)
	}
	// 3. RemoteAddr
	host, _, err := net.SplitHostPort(c.Request.RemoteAddr)
	if err != nil {
		return c.Request.RemoteAddr
	}
	return host
}

// isTrustedIP 判断 IP 是否在可信集合中，同时匹配 CIDR 网段
func isTrustedIP(ip string, trusted map[string]bool) bool {
	if trusted[ip] {
		return true
	}
	// 检查是否属于 127.0.0.0/8 网段
	parsed := net.ParseIP(ip)
	if parsed == nil {
		return false
	}
	if parsed.IsLoopback() {
		return true
	}
	// 检查是否匹配 CIDR 格式的可信配置（如 10.0.0.0/8）
	for k := range trusted {
		if strings.Contains(k, "/") {
			_, cidr, err := net.ParseCIDR(k)
			if err == nil && cidr.Contains(parsed) {
				return true
			}
		}
	}
	return false
}

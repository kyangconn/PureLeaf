// Package password 提供密码哈希与校验
package password

import "golang.org/x/crypto/bcrypt"

// Hash 对密码进行 bcrypt 哈希
func Hash(password string) (string, error) {
	bytes, err := bcrypt.GenerateFromPassword([]byte(password), bcrypt.DefaultCost)
	return string(bytes), err
}

// Verify 校验密码与哈希是否匹配
func Verify(password, hash string) bool {
	return bcrypt.CompareHashAndPassword([]byte(hash), []byte(password)) == nil
}

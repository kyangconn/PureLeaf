package service

import (
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"os"
	"path/filepath"
)

// BlobStore 是内容寻址的 blob 存储。
//
// 目录布局: {root}/{hash前2位}/{完整hash}
// 相同内容只存一份（sha256 去重）。blobPath 字段（存入 SQLite）是相对 root 的路径，
// 例如 "ab/cdef..."，这样数据根目录迁移时不破坏引用。
type BlobStore interface {
	// Put 写入内容（若已存在则跳过），返回 sha256 hex、相对 root 的 blob 路径、字节数。
	Put(content []byte) (hash, blobPath string, size int64, err error)
	// Get 按相对 root 的 blob 路径读取内容。
	Get(blobPath string) ([]byte, error)
	// Root 返回 blob 存储根目录（绝对路径）。
	Root() string
}

type blobStore struct {
	root string
}

// NewBlobStore 创建 blob 存储，root 为 blobs 目录绝对路径。
func NewBlobStore(root string) (BlobStore, error) {
	abs, err := filepath.Abs(root)
	if err != nil {
		return nil, fmt.Errorf("解析 blob 存储根目录失败: %w", err)
	}
	if err := os.MkdirAll(abs, 0755); err != nil {
		return nil, fmt.Errorf("创建 blob 存储目录失败: %w", err)
	}
	return &blobStore{root: abs}, nil
}

func (s *blobStore) Root() string { return s.root }

func (s *blobStore) Put(content []byte) (string, string, int64, error) {
	sum := sha256.Sum256(content)
	hash := hex.EncodeToString(sum[:])
	relPath := filepath.Join(hash[:2], hash)
	fullPath := filepath.Join(s.root, relPath)

	// 内容寻址去重：若 blob 已存在直接返回，不重复写。
	if info, err := os.Stat(fullPath); err == nil && !info.IsDir() {
		return hash, relPath, info.Size(), nil
	} else if err != nil && !os.IsNotExist(err) {
		return "", "", 0, fmt.Errorf("检查 blob 失败: %w", err)
	}

	if err := writeFileAtomic(fullPath, content, 0444); err != nil {
		return "", "", 0, fmt.Errorf("写入 blob 失败: %w", err)
	}
	return hash, relPath, int64(len(content)), nil
}

func (s *blobStore) Get(blobPath string) ([]byte, error) {
	// 防御：blobPath 必须是相对路径，禁止穿越 root。
	if filepath.IsAbs(blobPath) {
		return nil, fmt.Errorf("blob 路径不能是绝对路径")
	}
	fullPath := filepath.Join(s.root, blobPath)
	data, err := os.ReadFile(fullPath)
	if err != nil {
		return nil, fmt.Errorf("读取 blob 失败: %w", err)
	}
	return data, nil
}

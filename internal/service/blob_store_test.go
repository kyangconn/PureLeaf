package service

import (
	"os"
	"path/filepath"
	"testing"
)

func TestBlobStore_PutDeduplicates(t *testing.T) {
	dir := t.TempDir()
	store, err := NewBlobStore(filepath.Join(dir, "blobs"))
	if err != nil {
		t.Fatalf("NewBlobStore 失败: %v", err)
	}

	content := []byte("hello world")
	hash1, path1, size1, err := store.Put(content)
	if err != nil {
		t.Fatalf("首次 Put 失败: %v", err)
	}
	if size1 != int64(len(content)) {
		t.Errorf("size = %d, 期望 %d", size1, len(content))
	}

	// 相同内容再次 Put 应去重，返回相同的 hash/path，不产生新文件
	hash2, path2, size2, err := store.Put(content)
	if err != nil {
		t.Fatalf("二次 Put 失败: %v", err)
	}
	if hash1 != hash2 || path1 != path2 {
		t.Errorf("去重失败: hash/path 不一致")
	}
	if size1 != size2 {
		t.Errorf("size 不一致: %d vs %d", size1, size2)
	}

	// blob 文件应存在
	if _, err := os.Stat(filepath.Join(store.Root(), path1)); err != nil {
		t.Errorf("blob 文件不存在: %v", err)
	}
}

func TestBlobStore_PutDifferentContent(t *testing.T) {
	store, err := NewBlobStore(filepath.Join(t.TempDir(), "blobs"))
	if err != nil {
		t.Fatalf("NewBlobStore 失败: %v", err)
	}
	hash1, _, _, _ := store.Put([]byte("a"))
	hash2, _, _, _ := store.Put([]byte("b"))
	if hash1 == hash2 {
		t.Errorf("不同内容不应有相同 hash")
	}
}

func TestBlobStore_Get(t *testing.T) {
	store, err := NewBlobStore(filepath.Join(t.TempDir(), "blobs"))
	if err != nil {
		t.Fatalf("NewBlobStore 失败: %v", err)
	}
	content := []byte("read me")
	_, blobPath, _, _ := store.Put(content)

	got, err := store.Get(blobPath)
	if err != nil {
		t.Fatalf("Get 失败: %v", err)
	}
	if string(got) != string(content) {
		t.Errorf("内容不匹配")
	}
}

func TestBlobStore_GetRejectsAbsolutePath(t *testing.T) {
	store, _ := NewBlobStore(filepath.Join(t.TempDir(), "blobs"))
	if _, err := store.Get("/etc/passwd"); err == nil {
		t.Errorf("应拒绝绝对路径")
	}
}

func TestBlobStore_BlobPathIsRelative(t *testing.T) {
	store, _ := NewBlobStore(filepath.Join(t.TempDir(), "blobs"))
	hash, blobPath, _, _ := store.Put([]byte("rel"))
	if filepath.IsAbs(blobPath) {
		t.Errorf("blobPath 应为相对路径，得到 %q", blobPath)
	}
	// 前两位应是 hash 前缀的子目录
	expectedDir := hash[:2]
	dir := filepath.Dir(blobPath)
	if dir != expectedDir {
		t.Errorf("blobPath 目录 = %q, 期望 %q", dir, expectedDir)
	}
}

package service

import (
	"os"
	"path/filepath"
	"testing"
)

func TestWriteFileAtomicReplacesContent(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "main.tex")

	if err := os.WriteFile(path, []byte("old"), 0644); err != nil {
		t.Fatalf("write initial file: %v", err)
	}
	if err := writeFileAtomic(path, []byte("new"), 0644); err != nil {
		t.Fatalf("atomic write: %v", err)
	}

	data, err := os.ReadFile(path)
	if err != nil {
		t.Fatalf("read file: %v", err)
	}
	if string(data) != "new" {
		t.Fatalf("unexpected content: %q", string(data))
	}

	matches, err := filepath.Glob(filepath.Join(dir, ".goleaf-tmp-*"))
	if err != nil {
		t.Fatalf("glob temp files: %v", err)
	}
	if len(matches) != 0 {
		t.Fatalf("temp files were not cleaned up: %v", matches)
	}
}

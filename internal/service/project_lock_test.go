package service

import (
	"testing"
	"time"
)

func TestProjectLockManagerSerializesSameProject(t *testing.T) {
	manager := NewProjectLockManager()
	firstEntered := make(chan struct{})
	releaseFirst := make(chan struct{})
	firstDone := make(chan error, 1)

	go func() {
		firstDone <- manager.WithProjectLock(1, func() error {
			close(firstEntered)
			<-releaseFirst
			return nil
		})
	}()

	<-firstEntered

	secondEntered := make(chan struct{})
	secondDone := make(chan error, 1)
	go func() {
		secondDone <- manager.WithProjectLock(1, func() error {
			close(secondEntered)
			return nil
		})
	}()

	select {
	case <-secondEntered:
		t.Fatal("second operation entered before first lock was released")
	case <-time.After(50 * time.Millisecond):
	}

	close(releaseFirst)

	if err := <-firstDone; err != nil {
		t.Fatalf("first operation failed: %v", err)
	}

	select {
	case <-secondEntered:
	case <-time.After(time.Second):
		t.Fatal("second operation did not enter after first lock was released")
	}
	if err := <-secondDone; err != nil {
		t.Fatalf("second operation failed: %v", err)
	}
}

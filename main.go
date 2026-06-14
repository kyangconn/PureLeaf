package main

import (
	"embed"
	"log"

	"github.com/wailsapp/wails/v3/pkg/application"

	"github.com/kyangconn/goleaf/internal/bindings"
	"github.com/kyangconn/goleaf/internal/factory"
)

//go:embed all:web/dist
var assets embed.FS

func main() {
	container, err := factory.New()
	if err != nil {
		log.Fatal(err)
	}

	app := application.New(application.Options{
		Name:        "goleaf",
		Description: "Desktop LaTeX editor",
		Services: []application.Service{
			application.NewService(bindings.NewProjectService(container.ProjectSvc)),
			application.NewService(bindings.NewFileService(container.FileSvc)),
		},
		Assets: application.AssetOptions{
			Handler: application.AssetFileServerFS(assets),
		},
		Mac: application.MacOptions{
			ApplicationShouldTerminateAfterLastWindowClosed: true,
		},
		OnShutdown: func() {
			if err := container.Close(); err != nil {
				log.Printf("关闭资源失败: %v", err)
			}
		},
	})

	app.Window.NewWithOptions(application.WebviewWindowOptions{
		Name:             "main",
		Title:            "goleaf - LaTeX Editor",
		Width:            1280,
		Height:           800,
		MinWidth:         900,
		MinHeight:        600,
		Frameless:        true,
		BackgroundColour: application.NewRGB(27, 38, 54),
		URL:              "/",
		Mac: application.MacWindow{
			InvisibleTitleBarHeight: 48,
			TitleBar:                application.MacTitleBarHiddenInset,
		},
	})

	if err := app.Run(); err != nil {
		log.Fatal(err)
	}
}

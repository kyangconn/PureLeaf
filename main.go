package main

import (
	"embed"

	"github.com/wailsapp/wails/v2"
	"github.com/wailsapp/wails/v2/pkg/options"
	"github.com/wailsapp/wails/v2/pkg/options/assetserver"

	leafwails "github.com/kyangconn/goleaf/internal/transport"
)

//go:embed all:web/dist
var assets embed.FS

func main() {
	myapp := leafwails.New()

	err := wails.Run(&options.App{
		Title:     "goleaf — LaTeX Editor",
		Width:     1280,
		Height:    800,
		MinWidth:  900,
		MinHeight: 600,
		AssetServer: &assetserver.Options{
			Assets: assets,
		},
		Frameless:        true,
		BackgroundColour: &options.RGBA{R: 27, G: 38, B: 54, A: 1},
		OnStartup:        myapp.Startup,
		Bind: []interface{}{
			myapp,
		},
	})

	if err != nil {
		println("Error:", err.Error())
	}
}

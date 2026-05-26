import vue from "@vitejs/plugin-vue";
import { defineConfig } from "vite";

// Vite 配置 — 开发时代理 API 请求到 Go 后端
export default defineConfig({
  plugins: [vue()],
  server: {
    port: 5173,
    proxy: {
      "/api": {
        target: "http://localhost:8080",
        changeOrigin: true,
      },
    },
  },
  build: {
    outDir: "dist",
    assetsDir: "assets",
  },
});

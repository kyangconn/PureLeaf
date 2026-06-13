import vue from "@vitejs/plugin-vue";
import { defineConfig, loadEnv } from "vite";
import { fileURLToPath, URL } from "node:url";

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), "");
  const apiTarget = env.VITE_API_PROXY_TARGET || "http://localhost:8080";

  return {
    build: {
      emptyOutDir: true,
      outDir: "dist",
    },
    plugins: [vue()],
    resolve: {
      alias: {
        "@": fileURLToPath(new URL("./src", import.meta.url)),
        "@wailsjs": fileURLToPath(new URL("./wailsjs/go", import.meta.url)),
      },
    },
    server: {
      proxy: {
        "/api": {
          changeOrigin: true,
          target: apiTarget,
        },
      },
    },
    // css: {
    //   preprocessorOptions: {
    //     scss: {
    //       additionalData: '@use "@/styles/shared" as *;',
    //       api: "modern",
    //     },
    //   },
    // },
  };
});

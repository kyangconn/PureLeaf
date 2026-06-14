// 路由配置 — Wails 桌面端
import { createRouter, createWebHashHistory } from "vue-router";

const routes = [
  {
    component: () => import("../views/Home.vue"),
    name: "Home",
    path: "/",
  },
  {
    component: () => import("../views/Editor.vue"),
    name: "Editor",
    path: "/project/:id",
  },
  {
    component: () => import("../views/Settings.vue"),
    name: "Settings",
    path: "/settings",
  },
];

const router = createRouter({
  history: createWebHashHistory(),
  routes,
});

export default router;

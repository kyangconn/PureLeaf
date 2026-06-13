// 路由配置 — Wails 桌面端
import { createRouter, createWebHistory } from "vue-router";

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
];

const router = createRouter({
  history: createWebHistory(),
  routes,
});

export default router;

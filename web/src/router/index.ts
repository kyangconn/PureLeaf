// 路由配置 — Wails 桌面端
import { createRouter, createWebHistory } from "vue-router";

import { useAuthStore } from "../stores/auth";

const routes = [
  {
    component: () => import("../views/Setup.vue"),
    name: "Setup",
    path: "/setup",
  },
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

let setupChecked = false;

router.beforeEach(async (to) => {
  const auth = useAuthStore();

  if (!setupChecked) {
    setupChecked = true;
    if (await auth.checkSetup()) {
      return "/setup";
    }
    await auth.fetchUser();
  }

  if (to.path === "/setup" && auth.isLoggedIn && auth.user) {
    return "/";
  }
});

export default router;

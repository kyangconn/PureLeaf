// 路由配置
import { createRouter, createWebHistory } from "vue-router";
import { authAPI } from "../api";

const routes = [
  {
    path: "/setup",
    name: "Setup",
    component: () => import("../views/Setup.vue"),
    meta: { setup: true, plain: true },
  },
  {
    path: "/login",
    name: "Login",
    component: () => import("../views/Login.vue"),
    meta: { guest: true, plain: true },
  },
  {
    path: "/",
    name: "Home",
    component: () => import("../views/Home.vue"),
    meta: { requiresAuth: true },
  },
  {
    path: "/project/:id",
    name: "Editor",
    component: () => import("../views/Editor.vue"),
    meta: { requiresAuth: true },
  },
];

const router = createRouter({
  history: createWebHistory(),
  routes,
});

// ---- 工具函数 ----

function isLocalhost() {
  const hostname = window.location.hostname;
  return hostname === "localhost" || hostname === "127.0.0.1" || hostname === "[::1]" || hostname === "::1";
}

// ---- 全局状态 ----
let systemStatus = null; // null=未检查, { has_users: bool }

// 启动时检查系统是否需要初始化
async function checkSystemStatus() {
  if (systemStatus !== null) return systemStatus;
  try {
    const { data } = await authAPI.status();
    systemStatus = data;
  } catch {
    systemStatus = { has_users: true }; // 网络失败时假定已有用户，走正常登录
  }
  return systemStatus;
}

// 由 setup 页面调用，完成后重置状态缓存
export function resetSystemStatus() {
  systemStatus = null;
}

// 路由守卫
router.beforeEach(async (to, _from, next) => {
  const token = localStorage.getItem("token");

  // 首次访问时检查系统状态
  if (systemStatus === null) {
    await checkSystemStatus();
  }

  const needsSetup = systemStatus && !systemStatus.has_users;

  // 系统未初始化 → 仅 localhost 可访问 setup，其余跳登录
  if (needsSetup) {
    if (to.meta.setup) {
      if (isLocalhost()) {
        return next();
      }
      // 非 localhost 不允许 setup，跳登录页
      return next("/login");
    }
    // 非 setup 页但系统未初始化 → localhost 跳 setup，其余跳登录
    if (isLocalhost()) {
      return next("/setup");
    }
    return next("/login");
  }

  // 系统已初始化，setup 页不可再访问
  if (!needsSetup && to.meta.setup) {
    return next("/login");
  }

  // 需要登录但无 token → 跳登录
  if (to.meta.requiresAuth && !token) {
    return next("/login");
  }

  // 已登录访问游客页 → 跳首页
  if (to.meta.guest && token) {
    return next("/");
  }

  next();
});

export default router;

// 路由配置
import { createRouter, createWebHistory } from "vue-router";
import { authAPI } from "../api";

const routes = [
  {
    path: "/setup",
    name: "Setup",
    component: () => import("../views/Setup.vue"),
    meta: { setup: true },
  },
  {
    path: "/login",
    name: "Login",
    component: () => import("../views/Login.vue"),
    meta: { guest: true },
  },
  {
    path: "/register",
    name: "Register",
    component: () => import("../views/Register.vue"),
    meta: { guest: true },
  },
  {
    path: "/",
    name: "Dashboard",
    component: () => import("../views/Dashboard.vue"),
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

// 由认证页面调用，在注册/登录成功后重置状态缓存
// (否则 setup 后系统已有用户但缓存仍为 false，导致重定向死循环)
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

  // 系统未初始化 → 强制跳转 setup (setup 页本身和 API 调用除外)
  if (needsSetup && !to.meta.setup) {
    return next("/setup");
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

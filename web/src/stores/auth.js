// 认证状态管理 (Pinia)
import { defineStore } from "pinia";
import { ref, computed } from "vue";
import { authAPI } from "../api";

export const useAuthStore = defineStore("auth", () => {
  const token = ref(localStorage.getItem("token") || "");
  const user = ref(parseStoredUser());

  const isLoggedIn = computed(() => !!token.value);

  // 注册
  async function register(username, email, password) {
    const { data } = await authAPI.register({ username, email, password });
    token.value = data.token;
    user.value = data.user;
    localStorage.setItem("token", data.token);
    localStorage.setItem("user", JSON.stringify(data.user));
  }

  // 登录
  async function login(username, password) {
    const { data } = await authAPI.login({ username, password });
    token.value = data.token;
    user.value = data.user;
    localStorage.setItem("token", data.token);
    localStorage.setItem("user", JSON.stringify(data.user));
  }

  // 登出
  function logout() {
    token.value = "";
    user.value = null;
    localStorage.removeItem("token");
    localStorage.removeItem("user");
  }

  // 从服务端刷新用户信息
  async function fetchUser() {
    try {
      const { data } = await authAPI.me();
      user.value = data;
      localStorage.setItem("user", JSON.stringify(data));
    } catch {
      logout();
    }
  }

  return { token, user, isLoggedIn, register, login, logout, fetchUser };
});

function parseStoredUser() {
  try {
    const raw = localStorage.getItem("user");
    if (!raw || raw === "undefined") return null;
    return JSON.parse(raw);
  } catch {
    localStorage.removeItem("user"); // 清除损坏的缓存
    return null;
  }
}

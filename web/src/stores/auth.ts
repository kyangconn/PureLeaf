import { ref } from "vue";
import { defineStore } from "pinia";
import { domain } from "@wailsjs/models";

import { authAPI } from "../api";

export const useAuthStore = defineStore("auth", () => {
  const user = ref<domain.User | null>(null);
  const isLoggedIn = ref(true);

  async function fetchUser() {
    try {
      user.value = await authAPI.me();
    } catch {
      user.value = { username: "admin" } as domain.User;
    }
  }

  async function checkSetup() {
    try {
      const { has_users } = await authAPI.status();
      return !has_users;
    } catch {
      return false;
    }
  }

  return { checkSetup, fetchUser, isLoggedIn, user };
});

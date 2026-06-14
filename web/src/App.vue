<template>
  <!-- 带全局顶栏的页面 -->
  <BaseLayout
    :class="themeStore.appClasses"
    :dark="themeStore.isAppDark"
    @create-project="handleCreate"
    @open-settings="handleOpenSettings"
  >
    <router-view />
  </BaseLayout>
</template>

<script setup>
import { useRouter } from "vue-router";

import { projectAPI } from "./api";
import { useThemeStore } from "./stores/theme";
import BaseLayout from "./layouts/BaseLayout.vue";

const router = useRouter();
const themeStore = useThemeStore();

async function handleCreate() {
  try {
    const data = await projectAPI.create({ name: "未命名项目" });
    router.push({ path: `/project/${data.id}`, query: { open: "main.tex" } });
  } catch {
    /* handled */
  }
}

function handleOpenSettings() {
  router.push("/settings");
}
</script>

<template>
  <!-- 带全局顶栏的页面 -->
  <BaseLayout :dark="$route.name === 'Editor'" @create-project="handleCreate" @open-settings="handleOpenSettings">
    <router-view />
  </BaseLayout>
</template>

<script setup>
import { useRouter } from "vue-router";

import { projectAPI } from "./api";
import BaseLayout from "./layouts/BaseLayout.vue";

const router = useRouter();

async function handleCreate() {
  try {
    const data = await projectAPI.create({ name: "未命名项目" });
    router.push(`/project/${data.id}`);
  } catch {
    /* handled */
  }
}

function handleOpenSettings() {
  router.push("/settings");
}
</script>

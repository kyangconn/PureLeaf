<template>
  <div class="base-layout">
    <header class="layout-header" :class="{ 'layout-header--dark': dark }">
      <div class="header-left">
        <slot name="header-left">
          <span class="brand">goleaf</span>
        </slot>
      </div>
      <div class="header-center">
        <slot name="header-center" />
      </div>
      <div class="header-right">
        <slot name="header-right" />
        <el-button type="primary" size="small" :icon="Plus" @click="$emit('create-project')">新建</el-button>
        <el-dropdown trigger="click">
          <span class="user-name">{{ authStore.user?.username }}</span>
          <template #dropdown>
            <el-dropdown-menu>
              <el-dropdown-item @click="handleLogout">退出登录</el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </header>
    <main class="layout-main">
      <slot />
    </main>
  </div>
</template>

<script setup>
import { Plus } from "@element-plus/icons-vue";
import { useAuthStore } from "../stores/auth";

defineProps({
  dark: { type: Boolean, default: false },
});

defineEmits(["create-project"]);

const authStore = useAuthStore();

function handleLogout() {
  authStore.logout();
  window.location.href = "/login";
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.base-layout {
  @include full-page;
}

.layout-header {
  height: $header-height;
  @include flex-between;
  padding: 0 16px;
  background: #fff;
  border-bottom: 1px solid $color-border;
  flex-shrink: 0;

  &--dark {
    background: $color-panel-bg;
    border-bottom-color: $color-border-dark;
    color: $color-text-dark;
  }
}

.header-left {
  display: flex;
  align-items: center;
  gap: 8px;
  min-width: 180px;
}
.header-center {
  flex: 1;
  @include flex-center;
}
.header-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

.brand {
  font-size: 18px;
  font-weight: 700;
  color: $color-primary;
}

.user-name {
  font-size: 14px;
  color: #606266;
  cursor: pointer;
  .layout-header--dark & {
    color: $color-text-dark;
  }
}

.layout-main {
  flex: 1;
  @include flex-column;
  overflow: hidden;
}
</style>

<template>
  <div class="base-layout">
    <header class="layout-header" :class="{ 'layout-header--dark': dark }">
      <div class="header-left">
        <slot name="header-left">
          <span class="brand">goleaf</span>
        </slot>
      </div>
      <div class="header-right">
        <slot name="header-right">
          <el-dropdown trigger="click">
            <span class="user-name">{{ authStore.user?.username }}</span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item @click="handleLogout">退出登录</el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </slot>
      </div>
    </header>
    <main class="layout-main">
      <slot />
    </main>
  </div>
</template>

<script setup>
import { useAuthStore } from "../stores/auth";

defineProps({
  dark: { type: Boolean, default: false },
});

const authStore = useAuthStore();

function handleLogout() {
  authStore.logout();
  window.location.href = "/login";
}
</script>

<style scoped>
.base-layout {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.layout-header {
  height: var(--header-height);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 16px;
  background: #fff;
  border-bottom: 1px solid var(--color-border);
  flex-shrink: 0;
}

.layout-header--dark {
  background: var(--color-sidebar);
  border-bottom-color: var(--color-border-dark);
  color: #d4d4d4;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 8px;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

.brand {
  font-size: 18px;
  font-weight: 700;
  color: var(--color-primary);
}

.user-name {
  font-size: 14px;
  color: #606266;
  cursor: pointer;
}

.layout-header--dark .user-name {
  color: #d4d4d4;
}

.layout-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}
</style>

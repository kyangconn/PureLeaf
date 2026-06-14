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
        <el-button type="primary" :icon="Plus" class="create-project-btn" @click="$emit('create-project')">
          新建项目
        </el-button>
        <el-button
          :icon="Setting"
          class="settings-btn"
          title="设置"
          aria-label="设置"
          @click="$emit('open-settings')"
        />
        <div class="window-controls">
          <el-button text :icon="Minus" class="window-btn" title="最小化" aria-label="最小化" @click="minimize" />
          <el-button
            text
            :icon="FullScreen"
            class="window-btn"
            title="最大化或还原"
            aria-label="最大化或还原"
            @click="toggleMaximize"
          />
          <el-button text :icon="Close" class="window-btn close-btn" title="关闭" aria-label="关闭" @click="closeApp" />
        </div>
      </div>
    </header>
    <main class="layout-main">
      <slot />
    </main>
  </div>
</template>

<script setup>
import { Close, FullScreen, Minus, Plus, Setting } from "@element-plus/icons-vue";

import { Quit, WindowMinimise, WindowToggleMaximise } from "../../wailsjs/runtime/runtime";

defineProps({
  dark: { default: false, type: Boolean },
});

defineEmits(["create-project", "open-settings"]);

function hasWailsRuntime() {
  return Boolean(window.runtime);
}

function minimize() {
  if (hasWailsRuntime()) WindowMinimise();
}

function toggleMaximize() {
  if (hasWailsRuntime()) WindowToggleMaximise();
}

function closeApp() {
  if (hasWailsRuntime()) Quit();
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
  padding: 0 14px 0 16px;
  background: var(--app-header-bg);
  border-bottom: 1px solid var(--app-border);
  flex-shrink: 0;
  color: var(--app-text);
  --wails-draggable: drag;

  &--dark {
    background: var(--app-header-bg);
    border-bottom-color: var(--app-border);
    color: var(--app-text);
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
  --wails-draggable: no-drag;
}

.create-project-btn {
  height: 36px;
  padding: 0 16px;
  border: none;
  border-radius: 8px;
  background: var(--app-primary);
  color: var(--app-primary-contrast);
  font-weight: 600;

  &:hover,
  &:focus {
    background: var(--app-primary-hover);
    color: var(--app-primary-contrast);
  }
}

.settings-btn {
  width: 36px;
  height: 36px;
  padding: 0;
  border-color: var(--app-border);
  background: var(--app-surface);
  color: var(--app-text-secondary);

  &:hover,
  &:focus {
    border-color: var(--app-border-strong);
    background: var(--app-hover-bg);
    color: var(--app-text);
  }
}

.window-controls {
  display: flex;
  align-items: center;
  gap: 2px;
  margin-left: 4px;
}

.window-btn {
  width: 32px;
  height: 32px;
  padding: 0;
  border-radius: 6px;
  color: var(--app-text-secondary);

  &:hover {
    color: var(--app-text);
    background: var(--app-hover-bg);
  }
}

.close-btn:hover {
  color: #fff;
  background: #f56c6c;
}

.brand {
  font-size: 17px;
  font-weight: 800;
  color: var(--app-primary);
  letter-spacing: 0;
}

.layout-main {
  flex: 1;
  @include flex-column;
  overflow: hidden;
}
</style>

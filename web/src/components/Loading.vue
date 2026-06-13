<template>
  <div class="loading-wrapper" :class="{ 'is-fullscreen': fullscreen }">
    <div class="loading-content">
      <el-icon class="loading-icon is-loading" :size="fullscreen ? 40 : 32">
        <Loading />
      </el-icon>
      <p class="loading-text">
        <slot>加载中...</slot>
      </p>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Loading } from "@element-plus/icons-vue";

defineProps({
  fullscreen: {
    default: false,
    type: Boolean,
  },
});
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;

.loading-wrapper {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 32px 16px;
  opacity: 0;
  animation: fade-in 0.25s ease forwards;

  &.is-fullscreen {
    position: absolute;
    inset: 0;
    background: rgba(0, 0, 0, 0.45);
    z-index: 100;
    padding: 0;
  }
}

.loading-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
}

.loading-icon {
  color: $color-primary;
}

.loading-text {
  margin: 0;
  font-size: 14px;
  color: $color-text-secondary;
}

@keyframes fade-in {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}
</style>

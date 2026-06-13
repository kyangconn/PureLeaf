<template>
  <div class="auth-page">
    <div class="auth-card">
      <h1 class="auth-title">初次使用</h1>
      <p class="auth-subtitle">正在初始化 goleaf 工作环境...</p>
      <el-button type="primary" class="auth-btn" @click="$router.push('/')">开始使用</el-button>
    </div>
  </div>
</template>

<script setup>
// Setup 页现在只是一个欢迎页——后端已自动创建管理员账户
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.auth-page { @include auth-page; }
</style>

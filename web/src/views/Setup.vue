<template>
  <div class="auth-page">
    <div class="auth-card">
      <h1 class="auth-title">初次使用</h1>
      <p class="auth-subtitle">设置你的管理员账号，开始使用 goleaf</p>
      <el-form ref="formRef" :model="form" :rules="rules" size="large" @keyup.enter="handleSetup">
        <el-form-item prop="username">
          <el-input v-model="form.username" placeholder="设置用户名" :prefix-icon="User" />
        </el-form-item>
        <el-form-item prop="email">
          <el-input v-model="form.email" placeholder="设置邮箱" :prefix-icon="Message" />
        </el-form-item>
        <el-form-item prop="password">
          <el-input
            v-model="form.password"
            type="password"
            placeholder="设置密码 (至少6位)"
            show-password
            :prefix-icon="Lock"
          />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" class="auth-btn" :loading="loading" @click="handleSetup"> 完成设置 </el-button>
        </el-form-item>
      </el-form>
      <p class="auth-footer">仅初次启动时需要，后续请从登录页进入</p>
    </div>
  </div>
</template>

<script setup>
import { User, Lock, Message } from "@element-plus/icons-vue";
import { ref, reactive } from "vue";
import { useRouter } from "vue-router";
import { resetSystemStatus } from "../router";
import { useAuthStore } from "../stores/auth";

const router = useRouter();
const authStore = useAuthStore();

const formRef = ref(null);
const loading = ref(false);
const form = reactive({ username: "", email: "", password: "" });

const rules = {
  username: [{ required: true, message: "请设置用户名", trigger: "blur" }],
  email: [
    { required: true, message: "请设置邮箱", trigger: "blur" },
    { type: "email", message: "请输入有效的邮箱地址", trigger: "blur" },
  ],
  password: [
    { required: true, message: "请设置密码", trigger: "blur" },
    { min: 6, message: "密码至少6位", trigger: "blur" },
  ],
};

async function handleSetup() {
  const valid = await formRef.value.validate().catch(() => false);
  if (!valid) return;

  loading.value = true;
  try {
    await authStore.register(form.username, form.email, form.password);
    resetSystemStatus(); // setup 完成后刷新状态缓存
    router.push("/");
  } finally {
    loading.value = false;
  }
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.auth-page {
  @include auth-page;
}
</style>

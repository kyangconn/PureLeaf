<template>
  <div class="auth-page">
    <div class="auth-card">
      <h1 class="auth-title">goleaf</h1>
      <p class="auth-subtitle">轻量在线 LaTeX 编辑器</p>
      <el-form ref="formRef" :model="form" :rules="rules" size="large" @keyup.enter="handleLogin">
        <el-form-item prop="username">
          <el-input v-model="form.username" placeholder="用户名" :prefix-icon="User" />
        </el-form-item>
        <el-form-item prop="password">
          <el-input v-model="form.password" type="password" placeholder="密码" show-password :prefix-icon="Lock" />
        </el-form-item>
        <el-form-item>
          <el-button type="primary" class="auth-btn" :loading="loading" @click="handleLogin">登 录</el-button>
        </el-form-item>
      </el-form>
      <p class="auth-footer">还没有账号？<router-link to="/register">立即注册</router-link></p>
    </div>
  </div>
</template>

<script setup>
import { User, Lock } from "@element-plus/icons-vue";
import { ref, reactive } from "vue";
import { useRouter } from "vue-router";
import { resetSystemStatus } from "../router";
import { useAuthStore } from "../stores/auth";

const router = useRouter();
const authStore = useAuthStore();

const formRef = ref(null);
const loading = ref(false);
const form = reactive({ username: "", password: "" });

const rules = {
  username: [{ required: true, message: "请输入用户名", trigger: "blur" }],
  password: [{ required: true, message: "请输入密码", trigger: "blur" }],
};

async function handleLogin() {
  const valid = await formRef.value.validate().catch(() => false);
  if (!valid) return;

  loading.value = true;
  try {
    await authStore.login(form.username, form.password);
    resetSystemStatus();
    router.push("/");
  } finally {
    loading.value = false;
  }
}
</script>

<style scoped>
.auth-page {
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.auth-card {
  width: 400px;
  padding: 40px;
  background: #fff;
  border-radius: 12px;
  box-shadow: 0 20px 60px rgba(0, 0, 0, 0.15);
}

.auth-title {
  text-align: center;
  font-size: 32px;
  color: #303133;
  margin-bottom: 4px;
}

.auth-subtitle {
  text-align: center;
  color: #909399;
  font-size: 14px;
  margin-bottom: 32px;
}

.auth-btn {
  width: 100%;
}

.auth-footer {
  text-align: center;
  font-size: 14px;
  color: #909399;
}

.auth-footer a {
  color: #667eea;
  text-decoration: none;
}
</style>

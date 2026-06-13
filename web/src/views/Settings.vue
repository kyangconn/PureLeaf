<template>
  <div class="settings-page">
    <main class="settings-main">
      <div class="settings-toolbar">
        <h2>设置</h2>
        <el-button :icon="ArrowLeft" @click="router.push('/')">返回项目</el-button>
      </div>

      <section class="settings-section">
        <div class="section-header">
          <h3>编译</h3>
          <span>config.yaml</span>
        </div>
        <el-form label-width="96px" class="settings-form">
          <el-form-item label="编译器">
            <el-select v-model="compiler" disabled>
              <el-option label="pdflatex" value="pdflatex" />
              <el-option label="xelatex" value="xelatex" />
              <el-option label="lualatex" value="lualatex" />
            </el-select>
          </el-form-item>
          <el-form-item label="超时时间">
            <el-input-number v-model="timeout" disabled :min="1" :step="5" />
            <span class="field-unit">秒</span>
          </el-form-item>
        </el-form>
      </section>

      <section class="settings-section">
        <div class="section-header">
          <h3>编辑器</h3>
          <span>当前会话</span>
        </div>
        <el-form label-width="96px" class="settings-form">
          <el-form-item label="自动保存">
            <el-switch v-model="autoSave" disabled />
          </el-form-item>
          <el-form-item label="保存延迟">
            <el-input-number v-model="autoSaveDelay" disabled :min="500" :step="500" />
            <span class="field-unit">毫秒</span>
          </el-form-item>
        </el-form>
      </section>

      <section class="settings-section">
        <div class="section-header">
          <h3>界面</h3>
          <span>计划中</span>
        </div>
        <el-form label-width="96px" class="settings-form">
          <el-form-item label="主题">
            <el-segmented v-model="theme" disabled :options="themeOptions" />
          </el-form-item>
        </el-form>
      </section>
    </main>
  </div>
</template>

<script setup>
import { ref } from "vue";
import { useRouter } from "vue-router";
import { ArrowLeft } from "@element-plus/icons-vue";

defineOptions({
  name: "AppSettings",
});

const router = useRouter();

const compiler = ref("pdflatex");
const timeout = ref(60);
const autoSave = ref(true);
const autoSaveDelay = ref(2000);
const theme = ref("dark");
const themeOptions = [
  { label: "亮色", value: "light" },
  { label: "暗色", value: "dark" },
];
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.settings-page {
  height: 100%;
  background: $color-bg;
}

.settings-main {
  height: 100%;
  overflow-y: auto;
  padding: 24px;
  max-width: 860px;
  width: 100%;
  margin: 0 auto;
}

.settings-toolbar {
  @include flex-between;
  margin-bottom: 20px;

  h2 {
    font-size: 20px;
    font-weight: 600;
    color: $color-text;
  }
}

.settings-section {
  background: #fff;
  border: 1px solid $color-border;
  border-radius: 8px;
  margin-bottom: 16px;
  overflow: hidden;
}

.section-header {
  @include flex-between;
  min-height: 44px;
  padding: 0 16px;
  border-bottom: 1px solid $color-border;

  h3 {
    font-size: 15px;
    font-weight: 600;
    color: $color-text;
  }

  span {
    font-size: 12px;
    color: $color-text-secondary;
  }
}

.settings-form {
  padding: 18px 16px 4px;
}

.field-unit {
  margin-left: 10px;
  font-size: 13px;
  color: $color-text-secondary;
}
</style>

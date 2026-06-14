<template>
  <div class="settings-page">
    <main class="settings-main">
      <div class="settings-toolbar">
        <h2>设置</h2>
        <el-button :icon="ArrowLeft" @click="router.push('/')">返回项目</el-button>
      </div>

      <div class="settings-grid">
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
            <h3>界面</h3>
            <span>本地偏好</span>
          </div>
          <el-form label-width="96px" class="settings-form">
            <el-form-item label="应用主题">
              <el-segmented v-model="appTheme" :options="themeOptions" />
            </el-form-item>
          </el-form>
        </section>

        <section class="settings-section environment-section">
          <div class="section-header">
            <h3>LaTeX 环境</h3>
            <span>{{ latexStatus?.has_compiler ? "可用" : "未检测到" }}</span>
          </div>
          <el-form label-width="96px" class="settings-form">
            <el-form-item label="当前编译器">
              <div class="compiler-status">
                <el-tag :type="latexStatus?.has_compiler ? 'success' : 'warning'">
                  {{ latexStatus?.compiler || compiler }}
                </el-tag>
                <el-button text :icon="RefreshRight" :loading="checkingLatex" @click="reloadLatexEnvironment" />
              </div>
            </el-form-item>
            <el-form-item label="工具">
              <div class="tool-list">
                <div v-for="tool in latexStatus?.tools || []" :key="tool.name" class="tool-row">
                  <span class="tool-name">{{ tool.name }}</span>
                  <el-tag size="small" :type="tool.available ? 'success' : 'info'">
                    {{ tool.available ? "可用" : "缺失" }}
                  </el-tag>
                  <span class="tool-path" :title="tool.version || tool.path">
                    {{ tool.version || tool.path || "-" }}
                  </span>
                </div>
              </div>
            </el-form-item>
            <el-form-item label="TeX Live">
              <div class="texlive-panel">
                <el-segmented v-model="texLiveVariant" :options="texLiveOptions" class="texlive-schemes" />
                <div class="texlive-actions">
                  <el-button :icon="Download" :loading="downloadingInstaller" @click="downloadTexLive">
                    下载安装器
                  </el-button>
                  <el-button type="primary" :loading="startingInstaller" @click="startTexLiveInstaller">
                    启动安装
                  </el-button>
                </div>
                <div v-if="texLivePath" class="download-path" :title="texLivePath">{{ texLivePath }}</div>
              </div>
            </el-form-item>
          </el-form>
        </section>

        <section class="settings-section">
          <div class="section-header">
            <h3>编辑器</h3>
            <span>本地偏好</span>
          </div>
          <el-form label-width="96px" class="settings-form">
            <el-form-item label="编辑器主题">
              <el-segmented v-model="editorTheme" :options="themeOptions" />
            </el-form-item>
            <el-form-item label="自动保存">
              <el-switch v-model="autoSave" disabled />
            </el-form-item>
            <el-form-item label="保存延迟">
              <el-input-number v-model="autoSaveDelay" disabled :min="500" :step="500" />
              <span class="field-unit">毫秒</span>
            </el-form-item>
          </el-form>
        </section>
      </div>
    </main>
  </div>
</template>

<script setup>
import { storeToRefs } from "pinia";
import { ref, onMounted } from "vue";
import { useRouter } from "vue-router";
import { ElMessage } from "element-plus";
import { ArrowLeft, Download, RefreshRight } from "@element-plus/icons-vue";

import { latexAPI } from "../api";
import { useThemeStore } from "../stores/theme";

defineOptions({
  name: "AppSettings",
});

const router = useRouter();
const themeStore = useThemeStore();
const { appTheme, editorTheme } = storeToRefs(themeStore);

const compiler = ref("pdflatex");
const timeout = ref(60);
const autoSave = ref(true);
const autoSaveDelay = ref(2000);
const latexStatus = ref(null);
const checkingLatex = ref(false);
const downloadingInstaller = ref(false);
const startingInstaller = ref(false);
const texLiveVariant = ref("small");
const texLivePath = ref("");
const themeOptions = [
  { label: "亮色", value: "light" },
  { label: "暗色", value: "dark" },
];
const texLiveOptions = [
  { label: "Basic", value: "base" },
  { label: "Small", value: "small" },
  { label: "Medium", value: "medium" },
  { label: "Full", value: "full" },
];

onMounted(refreshLatexEnvironment);

async function refreshLatexEnvironment() {
  checkingLatex.value = true;
  try {
    latexStatus.value = await latexAPI.checkEnvironment();
  } finally {
    checkingLatex.value = false;
  }
}

async function reloadLatexEnvironment() {
  checkingLatex.value = true;
  try {
    latexStatus.value = await latexAPI.reloadEnvironment();
    ElMessage.success(latexStatus.value?.has_compiler ? "环境已刷新" : "环境已刷新，仍未检测到编译器");
  } finally {
    checkingLatex.value = false;
  }
}

async function downloadTexLive() {
  downloadingInstaller.value = true;
  try {
    const result = await latexAPI.downloadTexLiveInstaller(texLiveVariant.value);
    texLivePath.value = result.installer_path;
    ElMessage.success(result.already_exists ? "安装器已存在" : "下载完成");
  } finally {
    downloadingInstaller.value = false;
  }
}

async function startTexLiveInstaller() {
  startingInstaller.value = true;
  try {
    const result = await latexAPI.startTexLiveInstaller(texLiveVariant.value);
    texLivePath.value = result.installer_path;
    ElMessage.success(`已启动 ${result.scheme} 安装`);
  } finally {
    startingInstaller.value = false;
  }
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.settings-page {
  height: 100%;
  background: var(--app-bg);
}

.settings-main {
  height: 100%;
  overflow-y: auto;
  padding: 18px 22px 24px;
  width: 100%;
}

.settings-toolbar {
  @include flex-between;
  margin-bottom: 16px;

  h2 {
    font-size: 18px;
    font-weight: 700;
    color: var(--app-text);
  }
}

.settings-grid {
  display: grid;
  grid-template-columns: minmax(420px, 1fr) minmax(360px, 0.8fr);
  gap: 16px;
  align-items: start;
}

.settings-section {
  background: var(--app-surface);
  border: 1px solid var(--app-border);
  border-radius: 8px;
  box-shadow: var(--app-shadow);
  overflow: hidden;
}

.environment-section {
  grid-row: span 2;
}

.section-header {
  @include flex-between;
  min-height: 44px;
  padding: 0 16px;
  border-bottom: 1px solid var(--app-border);

  h3 {
    font-size: 15px;
    font-weight: 600;
    color: var(--app-text);
  }

  span {
    font-size: 12px;
    color: var(--app-text-secondary);
  }
}

.settings-form {
  padding: 18px 16px 4px;
}

.compiler-status {
  display: flex;
  align-items: center;
  gap: 8px;
}

.tool-list {
  width: 100%;
  border: 1px solid var(--app-border);
  border-radius: 8px;
  overflow: hidden;
}

.tool-row {
  display: grid;
  grid-template-columns: 104px 58px minmax(0, 1fr);
  gap: 10px;
  align-items: center;
  min-height: 34px;
  padding: 0 10px;
  border-bottom: 1px solid var(--app-border);

  &:last-child {
    border-bottom: none;
  }
}

.tool-name {
  font-size: 13px;
  font-weight: 600;
  color: var(--app-text);
}

.tool-path,
.download-path {
  color: var(--app-text-secondary);
  font-size: 12px;
  @include text-ellipsis;
}

.texlive-panel {
  min-width: 0;
}

.texlive-schemes {
  margin-bottom: 10px;
}

.texlive-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.download-path {
  margin-top: 8px;
}

.field-unit {
  margin-left: 10px;
  font-size: 13px;
  color: var(--app-text-secondary);
}

@media (max-width: 980px) {
  .settings-grid {
    grid-template-columns: 1fr;
  }

  .environment-section {
    grid-row: auto;
  }
}
</style>

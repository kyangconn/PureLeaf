<template>
  <div class="pdf-preview">
    <div class="pdf-header">
      <span>预览</span>
      <el-button text size="small" :icon="Close" @click="$emit('close')" />
    </div>
    <div class="pdf-content">
      <div v-if="log && !pdfUrl" class="compile-log">
        <div class="log-title">编译日志</div>
        <pre class="log-text">{{ log }}</pre>
      </div>
      <iframe v-if="pdfUrl" :src="pdfUrl" class="pdf-frame" frameborder="0" />
      <div v-if="!pdfUrl && !log" class="pdf-empty">
        <el-icon :size="48"><PictureFilled /></el-icon>
        <p>点击上方「编译」按钮生成 PDF 预览</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { Close, PictureFilled } from "@element-plus/icons-vue";

defineProps({
  log: { default: "", type: String },
  pdfUrl: { default: "", type: String },
});

defineEmits(["close"]);
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.pdf-preview {
  height: 100%;
  @include flex-column;
  background: var(--editor-panel-bg);
  color: var(--editor-text);
}

.pdf-header {
  @include panel-header(38px);
  background: var(--editor-panel-bg);

  :deep(.el-button) {
    color: var(--editor-text-dim);

    &:hover,
    &:focus {
      background: var(--editor-hover-bg);
      color: var(--editor-text);
    }
  }
}

.pdf-content {
  flex: 1;
  overflow: hidden;
  background: var(--editor-code-bg);
}

.pdf-frame {
  width: 100%;
  height: 100%;
  border: none;
}

.pdf-empty {
  height: 100%;
  @include flex-center;
  flex-direction: column;
  color: var(--editor-text-dim);
  gap: 12px;
  p {
    font-size: 13px;
  }
}

.compile-log {
  height: 100%;
  @include flex-column;
  padding: 12px;
  overflow-y: auto;
  background: var(--editor-code-bg);
}

.log-title {
  font-size: 13px;
  font-weight: 600;
  margin-bottom: 8px;
  color: var(--editor-text);
}

.log-text {
  flex: 1;
  font-size: 12px;
  font-family: Consolas, "Courier New", monospace;
  white-space: pre-wrap;
  word-break: break-all;
  color: var(--editor-text-dim);
  line-height: 1.5;
}
</style>

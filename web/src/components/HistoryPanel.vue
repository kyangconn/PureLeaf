<template>
  <div class="history-panel">
    <div class="panel-toolbar">
      <span class="panel-title">历史</span>
      <div class="panel-actions">
        <el-tooltip content="刷新" placement="bottom">
          <el-button text size="small" :icon="RefreshRight" :loading="loading" @click="loadHistory" />
        </el-tooltip>
      </div>
    </div>

    <div class="panel-body">
      <div v-if="loading && revisions.length === 0" class="panel-empty">加载中...</div>
      <div v-else-if="revisions.length === 0" class="panel-empty">
        暂无历史版本
        <span class="empty-hint">编辑并保存文件后会自动记录</span>
      </div>
      <template v-else>
        <ul class="history-list">
          <li
            v-for="rev in revisions"
            :key="rev.id"
            class="history-item"
            :class="{ selected: selectedA === rev.id, 'selected-b': selectedB === rev.id }"
            @click="onSelect(rev)"
          >
            <div class="item-marker">
              <span v-if="selectedA === rev.id" class="marker-a">A</span>
              <span v-else-if="selectedB === rev.id" class="marker-b">B</span>
              <span v-else class="marker-dot" :class="rev.reason" />
            </div>
            <div class="item-main">
              <span class="item-path" :title="rev.file_path">{{ shortName(rev.file_path) }}</span>
              <span class="item-meta"> {{ formatTime(rev.created_at) }} · {{ formatSize(rev.size) }} </span>
            </div>
          </li>
        </ul>

        <div v-if="diffText || viewContent" class="history-detail">
          <div class="detail-header">
            <span class="detail-title">{{ diffText ? "版本对比" : "版本内容" }}</span>
            <el-button text size="small" :icon="Close" @click="clearDetail" />
          </div>
          <pre v-if="diffText" class="detail-diff">{{ diffText }}</pre>
          <pre v-else class="detail-content">{{ viewContent }}</pre>
        </div>
      </template>
    </div>
  </div>
</template>

<script setup>
import { ref, watch } from "vue";
import { Close, RefreshRight } from "@element-plus/icons-vue";

import { fileAPI } from "../api";

const props = defineProps({
  projectId: { required: true, type: Number },
  refreshKey: { default: 0, type: Number },
});

const loading = ref(false);
const revisions = ref([]);
const selectedA = ref(null);
const selectedB = ref(null);
const diffText = ref("");
const viewContent = ref("");

watch(
  () => [props.projectId, props.refreshKey],
  () => loadHistory(),
  { immediate: true },
);

async function loadHistory() {
  if (!props.projectId) return;
  loading.value = true;
  try {
    revisions.value = (await fileAPI.getProjectHistory(props.projectId)) || [];
    // 重置选择（旧版本可能已不在列表）
    if (selectedA.value && !revisions.value.some((r) => r.id === selectedA.value)) selectedA.value = null;
    if (selectedB.value && !revisions.value.some((r) => r.id === selectedB.value)) selectedB.value = null;
  } finally {
    loading.value = false;
  }
}

async function onSelect(rev) {
  diffText.value = "";
  viewContent.value = "";
  // 第一次点：选 A；第二次点：选 B 并对比；第三次点：重置为 A
  if (selectedA.value === null) {
    selectedA.value = rev.id;
    const res = await fileAPI.getRevisionContent(rev.id);
    viewContent.value = res?.content || "(空)";
  } else if (selectedB.value === null && rev.id !== selectedA.value) {
    selectedB.value = rev.id;
    const res = await fileAPI.diffRevisions(selectedA.value, rev.id);
    diffText.value = res?.diff || "(无差异)";
  } else {
    // 重新开始
    selectedA.value = rev.id;
    selectedB.value = null;
    const res = await fileAPI.getRevisionContent(rev.id);
    viewContent.value = res?.content || "(空)";
  }
}

function clearDetail() {
  diffText.value = "";
  viewContent.value = "";
  selectedA.value = null;
  selectedB.value = null;
}

function shortName(path) {
  if (!path) return "";
  const parts = path.split(/[\\/]/);
  return parts[parts.length - 1] || path;
}

function formatTime(iso) {
  if (!iso) return "";
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return iso;
  const pad = (n) => String(n).padStart(2, "0");
  return `${d.getMonth() + 1}/${d.getDate()} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
}

function formatSize(bytes) {
  if (!bytes) return "0 B";
  if (bytes < 1024) return `${bytes} B`;
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.history-panel {
  height: 100%;
  @include flex-column;
}

.panel-toolbar {
  @include panel-header(36px);
}

.panel-actions {
  display: flex;
  gap: 2px;
}

.panel-body {
  flex: 1;
  min-height: 0;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.panel-empty {
  flex: 1;
  @include flex-center;
  flex-direction: column;
  gap: 4px;
  color: var(--editor-text-dim);
  font-size: 13px;

  .empty-hint {
    font-size: 11px;
    opacity: 0.7;
  }
}

.history-list {
  list-style: none;
  margin: 0;
  padding: 6px 0;
  overflow-y: auto;
  flex-shrink: 0;
  max-height: 45%;

  @include custom-scrollbar;
}

.history-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 12px;
  cursor: pointer;
  transition: background 0.15s;

  &:hover {
    background: var(--editor-hover-bg);
  }

  &.selected,
  &.selected-b {
    background: var(--editor-accent-soft);
  }
}

.item-marker {
  width: 18px;
  flex-shrink: 0;
  display: flex;
  justify-content: center;
}

.marker-a,
.marker-b {
  width: 18px;
  height: 18px;
  border-radius: 4px;
  font-size: 11px;
  font-weight: 700;
  @include flex-center;
  color: var(--app-primary-contrast);
}

.marker-a {
  background: var(--editor-accent);
}

.marker-b {
  background: var(--app-primary-hover, var(--editor-accent));
}

.marker-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--editor-text-dim);

  &.save {
    background: var(--editor-accent);
  }

  &.delete {
    background: var(--el-color-danger, #f56c6c);
  }
}

.item-main {
  flex: 1;
  min-width: 0;
  @include flex-column;
  gap: 2px;
}

.item-path {
  font-size: 13px;
  color: var(--editor-text);
  @include text-ellipsis;
}

.item-meta {
  font-size: 11px;
  color: var(--editor-text-dim);
}

.history-detail {
  flex: 1;
  min-height: 0;
  @include flex-column;
  border-top: 1px solid var(--editor-border);
  background: var(--editor-code-bg);
}

.detail-header {
  @include panel-header(34px);
  background: var(--editor-panel-bg);
}

.detail-title {
  font-size: 12px;
}

.detail-diff,
.detail-content {
  flex: 1;
  margin: 0;
  padding: 8px 12px;
  overflow: auto;
  font-family: "Cascadia Code", "JetBrains Mono", Consolas, monospace;
  font-size: 12px;
  line-height: 1.5;
  color: var(--editor-text);
  white-space: pre-wrap;
  word-break: break-word;

  @include custom-scrollbar;
}
</style>

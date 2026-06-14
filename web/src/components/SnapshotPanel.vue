<template>
  <div class="snapshot-panel">
    <div class="panel-toolbar">
      <span class="panel-title">快照</span>
      <div class="panel-actions">
        <el-tooltip content="刷新" placement="bottom">
          <el-button text size="small" :icon="RefreshRight" :loading="loading" @click="loadSnapshots" />
        </el-tooltip>
      </div>
    </div>

    <div class="panel-body">
      <div v-if="loading && snapshots.length === 0" class="panel-empty">加载中...</div>
      <div v-else-if="snapshots.length === 0" class="panel-empty">
        暂无快照
        <span class="empty-hint">删除文件或项目时会自动生成安全快照</span>
      </div>
      <ul v-else class="snapshot-list">
        <li v-for="snap in snapshots" :key="snap.id" class="snapshot-item">
          <div class="snap-icon">
            <el-icon :size="16"><Camera /></el-icon>
          </div>
          <div class="snap-main">
            <span class="snap-title">{{ reasonLabel(snap.reason) }} · {{ snap.file_count }} 个文件</span>
            <span class="snap-meta"> {{ formatTime(snap.created_at) }} · {{ formatSize(snap.total_size) }} </span>
            <span v-if="snap.snapshot_of" class="snap-target" :title="snap.snapshot_of">{{ snap.snapshot_of }}</span>
          </div>
        </li>
      </ul>
    </div>
  </div>
</template>

<script setup>
import { ref, watch } from "vue";
import { Camera, RefreshRight } from "@element-plus/icons-vue";

import { fileAPI } from "../api";

const props = defineProps({
  projectId: { required: true, type: Number },
  refreshKey: { default: 0, type: Number },
});

const loading = ref(false);
const snapshots = ref([]);

watch(
  () => [props.projectId, props.refreshKey],
  () => loadSnapshots(),
  { immediate: true },
);

async function loadSnapshots() {
  if (!props.projectId) return;
  loading.value = true;
  try {
    snapshots.value = (await fileAPI.getProjectSnapshots(props.projectId)) || [];
  } finally {
    loading.value = false;
  }
}

function reasonLabel(reason) {
  switch (reason) {
    case "delete":
      return "删除前快照";
    case "reset":
      return "重置前快照";
    case "checkout":
      return "切换前快照";
    default:
      return "快照";
  }
}

function formatTime(iso) {
  if (!iso) return "";
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return iso;
  const pad = (n) => String(n).padStart(2, "0");
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
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

.snapshot-panel {
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
  overflow-y: auto;

  @include custom-scrollbar;
}

.panel-empty {
  height: 100%;
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

.snapshot-list {
  list-style: none;
  margin: 0;
  padding: 6px 0;
}

.snapshot-item {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  padding: 8px 12px;
  transition: background 0.15s;

  &:hover {
    background: var(--editor-hover-bg);
  }
}

.snap-icon {
  flex-shrink: 0;
  color: var(--editor-accent);
  padding-top: 1px;
}

.snap-main {
  flex: 1;
  min-width: 0;
  @include flex-column;
  gap: 2px;
}

.snap-title {
  font-size: 13px;
  font-weight: 550;
  color: var(--editor-text);
}

.snap-meta {
  font-size: 11px;
  color: var(--editor-text-dim);
}

.snap-target {
  font-size: 11px;
  color: var(--editor-text-dim);
  @include text-ellipsis;
  opacity: 0.8;
}
</style>

<template>
  <div class="project-card" @click="$emit('select', project)">
    <div class="project-icon">
      <el-icon :size="32"><Document /></el-icon>
    </div>
    <div class="project-info">
      <div class="project-name">{{ project.name }}</div>
      <div class="project-date">{{ formatDate(project.updated_at) }}</div>
    </div>
    <div class="project-actions" @click.stop>
      <el-dropdown trigger="click">
        <el-button text :icon="MoreFilled" />
        <template #dropdown>
          <el-dropdown-menu>
            <el-dropdown-item @click="$emit('rename', project)">重命名</el-dropdown-item>
            <el-dropdown-item divided @click="$emit('delete', project)">删除</el-dropdown-item>
          </el-dropdown-menu>
        </template>
      </el-dropdown>
    </div>
  </div>
</template>

<script setup>
import { Document, MoreFilled } from "@element-plus/icons-vue";

defineProps({
  project: { type: Object, required: true },
});

defineEmits(["select", "rename", "delete"]);

function formatDate(dateStr) {
  if (!dateStr) return "";
  const d = new Date(dateStr);
  return d.toLocaleString("zh-CN", {
    year: "numeric",
    month: "2-digit",
    day: "2-digit",
    hour: "2-digit",
    minute: "2-digit",
  });
}
</script>

<style scoped>
.project-card {
  background: #fff;
  border-radius: 8px;
  padding: 20px;
  display: flex;
  align-items: center;
  gap: 16px;
  cursor: pointer;
  transition:
    box-shadow 0.2s,
    transform 0.2s;
  border: 1px solid var(--color-border);
}

.project-card:hover {
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.08);
  transform: translateY(-2px);
}

.project-icon {
  width: 48px;
  height: 48px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--color-primary-light);
  border-radius: 8px;
  color: var(--color-primary);
  flex-shrink: 0;
}

.project-info {
  flex: 1;
  min-width: 0;
}

.project-name {
  font-size: 15px;
  font-weight: 500;
  color: var(--color-text);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-date {
  font-size: 12px;
  color: var(--color-text-secondary);
  margin-top: 4px;
}

.project-actions {
  flex-shrink: 0;
}
</style>

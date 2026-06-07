<template>
  <div class="project-list">
    <div v-for="project in projects" :key="project.id" class="project-row" @click="$emit('select', project)">
      <div class="project-icon">
        <el-icon :size="24"><Document /></el-icon>
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
    <div v-if="projects.length === 0" class="project-list-empty">
      <el-empty description="暂无项目" :image-size="80" />
    </div>
  </div>
</template>

<script setup>
import { Document, MoreFilled } from "@element-plus/icons-vue";

defineProps({
  projects: { type: Array, required: true },
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

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.project-list {
  background: #fff;
  border: 1px solid $color-border;
  border-radius: 8px;
  overflow: hidden;
}

.project-row {
  display: flex;
  align-items: center;
  height: 52px;
  padding: 0 16px;
  cursor: pointer;
  transition: background 0.15s;
  border-bottom: 1px solid $color-border;
  &:last-child {
    border-bottom: none;
  }
  &:hover {
    background: $color-bg;
  }
}

.project-icon {
  @include flex-center;
  color: $color-primary;
  flex-shrink: 0;
  margin-right: 12px;
}

.project-info {
  flex: 1;
  min-width: 0;
}

.project-name {
  font-size: 14px;
  font-weight: 500;
  color: $color-text;
  @include text-ellipsis;
  line-height: 1.4;
}

.project-date {
  font-size: 12px;
  color: $color-text-secondary;
  line-height: 1.4;
}

.project-actions {
  flex-shrink: 0;
  margin-left: 8px;
}
.project-list-empty {
  padding: 40px 0;
}
</style>

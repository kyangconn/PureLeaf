<template>
  <div class="tree-node">
    <div
      class="node-row"
      :class="{ active: activeId === node.id, 'is-dir': node.is_dir }"
      :style="{ paddingLeft: depth * 16 + 8 + 'px' }"
      @click="$emit('select', node)"
    >
      <!-- 展开/折叠箭头 (仅目录) -->
      <span class="node-arrow" @click.stop="expanded = !expanded">
        <el-icon v-if="node.is_dir" :size="12">
          <ArrowRight v-if="!expanded" />
          <ArrowDown v-else />
        </el-icon>
        <span v-else style="width: 12px; display: inline-block" />
      </span>

      <!-- 图标 -->
      <el-icon :size="14" class="node-icon">
        <Folder v-if="node.is_dir && !expanded" />
        <FolderOpened v-else-if="node.is_dir" />
        <Document v-else />
      </el-icon>

      <!-- 文件名 -->
      <span class="node-name" :title="node.name">{{ node.name }}</span>

      <!-- 操作菜单 -->
      <div class="node-actions" @click.stop>
        <el-dropdown trigger="click" :hide-on-click="false">
          <el-button text size="small" :icon="MoreFilled" class="action-btn" />
          <template #dropdown>
            <el-dropdown-menu>
              <!-- 目录允许在此目录下创建文件 -->
              <template v-if="node.is_dir">
                <el-dropdown-item @click="actions.openCreateDialog(false, node.id)">新建文件</el-dropdown-item>
                <el-dropdown-item @click="actions.openCreateDialog(true, node.id)">新建文件夹</el-dropdown-item>
                <el-dropdown-item divided @click="actions.openRenameDialog(node.id, node.name)"
                  >重命名</el-dropdown-item
                >
              </template>
              <template v-else>
                <el-dropdown-item @click="actions.openRenameDialog(node.id, node.name)">重命名</el-dropdown-item>
              </template>
              <el-dropdown-item divided class="danger-item" @click="handleDelete">删除</el-dropdown-item>
            </el-dropdown-menu>
          </template>
        </el-dropdown>
      </div>
    </div>

    <!-- 子节点 (仅目录可见且展开时) -->
    <template v-if="node.is_dir && expanded && node.children">
      <TreeNode
        v-for="child in node.children"
        :key="child.id"
        :node="child"
        :depth="depth + 1"
        :active-id="activeId"
        @select="(n) => $emit('select', n)"
        @rename="(id, name) => $emit('rename', id, name)"
        @delete="(id) => $emit('delete', id)"
      />
    </template>
  </div>
</template>

<script setup>
import { ref, inject } from "vue";
import { ElMessageBox } from "element-plus";
import { ArrowRight, ArrowDown, Folder, FolderOpened, Document, MoreFilled } from "@element-plus/icons-vue";

const props = defineProps({
  activeId: { default: null, type: Number },
  depth: { default: 0, type: Number },
  node: { required: true, type: Object },
});

const emit = defineEmits(["select", "rename", "delete"]);

const expanded = ref(props.node.is_dir ? true : undefined);

// 从 FileTree 注入的操作方法
const actions = inject("fileTreeActions", {
  openCreateDialog: () => {},
  openRenameDialog: () => {},
});

async function handleDelete() {
  try {
    await ElMessageBox.confirm(
      `确定要删除「${props.node.name}」吗？${props.node.is_dir ? "其中的所有文件也将被删除。" : ""}`,
      "确认删除",
      { cancelButtonText: "取消", confirmButtonText: "删除", type: "warning" },
    );
    emit("delete", props.node.id);
  } catch {
    /* cancelled */
  }
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.tree-node {
  user-select: none;
}

.node-row {
  display: flex;
  align-items: center;
  height: 30px;
  padding-right: 4px;
  border-radius: 6px;
  cursor: pointer;
  font-size: 13px;
  color: var(--editor-text);
  transition: background 0.1s;

  &:hover {
    background: var(--editor-hover-bg);
  }
  &.active {
    background: var(--editor-active-bg);
  }
  &.active .node-name {
    color: var(--editor-active-text);
    font-weight: 600;
  }
}

.node-arrow {
  width: 16px;
  @include flex-center;
  flex-shrink: 0;
  color: var(--editor-icon);
}

.node-icon {
  margin-right: 6px;
  flex-shrink: 0;
  color: var(--editor-icon);
}

.node-row.is-dir .node-icon {
  color: var(--editor-dir);
}

.node-name {
  flex: 1;
  @include text-ellipsis;
}

.node-actions {
  display: flex;
  opacity: 0;
  flex-shrink: 0;
  transition: opacity 0.12s;
}

.node-row:hover .node-actions {
  opacity: 1;
}

.action-btn {
  width: 24px;
  height: 24px;
  padding: 2px;
  color: var(--editor-icon);

  &:hover,
  &:focus {
    background: var(--editor-hover-bg);
    color: var(--editor-text);
  }
}
.danger-item {
  color: #f56c6c;
}
</style>

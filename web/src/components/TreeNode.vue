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
                <el-dropdown-item
                  @click="actions.openCreateDialog(false, node.id)"
                  >新建文件</el-dropdown-item
                >
                <el-dropdown-item
                  @click="actions.openCreateDialog(true, node.id)"
                  >新建文件夹</el-dropdown-item
                >
                <el-dropdown-item
                  divided
                  @click="actions.openRenameDialog(node.id, node.name)"
                  >重命名</el-dropdown-item
                >
              </template>
              <template v-else>
                <el-dropdown-item
                  @click="actions.openRenameDialog(node.id, node.name)"
                  >重命名</el-dropdown-item
                >
              </template>
              <el-dropdown-item
                divided
                class="danger-item"
                @click="handleDelete"
                >删除</el-dropdown-item
              >
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
import {
  ArrowRight,
  ArrowDown,
  Folder,
  FolderOpened,
  Document,
  MoreFilled,
} from "@element-plus/icons-vue";
import { ElMessageBox } from "element-plus";
import { ref, inject } from "vue";

const props = defineProps({
  node: { type: Object, required: true },
  depth: { type: Number, default: 0 },
  activeId: { type: Number, default: null },
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
      { type: "warning", confirmButtonText: "删除", cancelButtonText: "取消" },
    );
    emit("delete", props.node.id);
  } catch {
    /* cancelled */
  }
}
</script>

<style scoped>
.tree-node {
  user-select: none;
}

.node-row {
  display: flex;
  align-items: center;
  height: 28px;
  padding-right: 4px;
  cursor: pointer;
  font-size: 13px;
  color: #ccc;
  transition: background 0.1s;
}

.node-row:hover {
  background: #2a2d2e;
}

.node-row.active {
  background: #37373d;
}

.node-row.active .node-name {
  color: #fff;
}

.node-arrow {
  width: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
  color: #888;
}

.node-icon {
  margin-right: 4px;
  flex-shrink: 0;
  color: #888;
}

.node-row.is-dir .node-icon {
  color: #dcb67a;
}

.node-name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.node-actions {
  display: none;
  flex-shrink: 0;
}

.node-row:hover .node-actions {
  display: flex;
}

.action-btn {
  padding: 2px;
  color: #888;
}

.danger-item {
  color: #f56c6c;
}
</style>

<template>
  <div class="file-tree">
    <!-- 工具栏 -->
    <div class="tree-toolbar">
      <span class="tree-title">文件</span>
      <div class="tree-actions">
        <el-button
          text
          size="small"
          :icon="DocumentAdd"
          title="新建文件"
          @click="openCreateDialog(false, null)"
        />
        <el-button
          text
          size="small"
          :icon="FolderAdd"
          title="新建文件夹"
          @click="openCreateDialog(true, null)"
        />
      </div>
    </div>

    <!-- 树形列表 -->
    <div class="tree-list">
      <div v-if="!files || files.length === 0" class="tree-empty">
        暂无文件，点击上方图标创建
      </div>

      <template v-for="node in files" :key="node.id">
        <TreeNode
          :node="node"
          :depth="0"
          :active-id="activeFileId"
          @select="$emit('select', $event)"
          @rename="(id, name) => $emit('rename', id, name)"
          @delete="(id) => $emit('delete', id)"
        />
      </template>
    </div>

    <!-- 新建对话框 -->
    <el-dialog
      v-model="showCreateDialog"
      :title="createIsDir ? '新建文件夹' : '新建文件'"
      width="360px"
    >
      <el-form
        ref="createFormRef"
        :model="createForm"
        :rules="createRules"
        @keyup.enter="confirmCreate"
      >
        <el-form-item prop="name">
          <el-input
            v-model="createForm.name"
            :placeholder="createIsDir ? '文件夹名称' : '文件名 (如 main.tex)'"
          />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" @click="confirmCreate">创建</el-button>
      </template>
    </el-dialog>

    <!-- 重命名对话框 -->
    <el-dialog v-model="showRenameDialog" title="重命名" width="360px">
      <el-form
        ref="renameFormRef"
        :model="renameForm"
        :rules="renameRules"
        @keyup.enter="confirmRename"
      >
        <el-form-item prop="name">
          <el-input v-model="renameForm.name" placeholder="新名称" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showRenameDialog = false">取消</el-button>
        <el-button type="primary" @click="confirmRename">确认</el-button>
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { DocumentAdd, FolderAdd } from "@element-plus/icons-vue";
import { ref, reactive, provide } from "vue";
import TreeNode from "./TreeNode.vue";

const props = defineProps({
  files: { type: Array, default: () => [] },
  activeFileId: { type: Number, default: null },
});

const emit = defineEmits(["select", "createFile", "rename", "delete"]);

// ---- 新建 ----
const showCreateDialog = ref(false);
const createIsDir = ref(false);
const createParentId = ref(null);
const createFormRef = ref(null);
const createForm = reactive({ name: "" });
const createRules = {
  name: [{ required: true, message: "请输入名称", trigger: "blur" }],
};

function openCreateDialog(isDir, parentId) {
  createIsDir.value = isDir;
  createParentId.value = parentId;
  createForm.name = "";
  showCreateDialog.value = true;
}

function confirmCreate() {
  createFormRef.value?.validate((valid) => {
    if (!valid) return;
    emit(
      "createFile",
      createForm.name,
      createParentId.value,
      createIsDir.value,
    );
    showCreateDialog.value = false;
  });
}

// ---- 重命名 ----
const showRenameDialog = ref(false);
const renameFileId = ref(null);
const renameFormRef = ref(null);
const renameForm = reactive({ name: "" });
const renameRules = {
  name: [{ required: true, message: "请输入新名称", trigger: "blur" }],
};

function openRenameDialog(fileId, currentName) {
  renameFileId.value = fileId;
  renameForm.name = currentName;
  showRenameDialog.value = true;
}

function confirmRename() {
  renameFormRef.value?.validate((valid) => {
    if (!valid) return;
    emit("rename", renameFileId.value, renameForm.name);
    showRenameDialog.value = false;
  });
}

// 通过 provide/inject 暴露给子孙 TreeNode 组件
provide("fileTreeActions", {
  openCreateDialog,
  openRenameDialog,
});
</script>

<style scoped>
.file-tree {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.tree-toolbar {
  height: 35px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 10px;
  border-bottom: 1px solid #3c3c3c;
}

.tree-title {
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  color: #999;
  font-weight: 600;
}

.tree-actions {
  display: flex;
  gap: 2px;
}

.tree-list {
  flex: 1;
  overflow-y: auto;
  padding: 4px 0;
}

.tree-empty {
  padding: 16px;
  text-align: center;
  color: #999;
  font-size: 12px;
}
</style>

<template>
  <div class="editor-page">
    <!-- 次顶栏：返回 / 项目名 / 编译 -->
    <div class="editor-toolbar">
      <el-button text :icon="ArrowLeft" @click="$router.push('/')">返回</el-button>

      <div class="project-title-bar">
        <span v-if="!renamingProject" class="project-title" @dblclick="startRenameProject">
          {{ project?.name || "加载中..." }}
        </span>
        <el-input
          v-else
          ref="renameInputRef"
          v-model="renameProjectName"
          size="small"
          class="project-rename-input"
          @blur="confirmRenameProject"
          @keyup.enter="confirmRenameProject"
        />
        <el-button v-if="!renamingProject" text size="small" :icon="EditPen" @click="startRenameProject" />
      </div>

      <div class="toolbar-actions">
        <el-button type="primary" size="small" :icon="Upload" :loading="compiling" @click="handleCompile">
          {{ compiling ? "编译中..." : "编译" }}
        </el-button>
        <el-button size="small" :icon="RefreshRight" @click="refreshTree">刷新</el-button>
      </div>
    </div>

    <!-- 主体三栏 -->
    <div class="editor-body">
      <aside class="sidebar" :style="{ width: sidebarWidth + 'px' }">
        <FileTree
          :files="fileTree"
          :active-file-id="activeFile?.id"
          @select="handleFileSelect"
          @create-file="handleCreateFile"
          @rename="handleRenameFile"
          @delete="handleDeleteFile"
        />
      </aside>

      <div class="resizer" @mousedown="startResize('sidebar', $event)"></div>

      <main class="editor-main">
        <div v-if="!activeFile" class="editor-empty">
          <el-empty description="从左侧文件树选择一个文件开始编辑" />
        </div>
        <div v-else class="editor-wrapper">
          <div class="editor-tab-bar">
            <span class="editor-tab active">
              <span class="tab-icon">📄</span>
              {{ activeFile.name }}
              <el-button text size="small" class="tab-close" :icon="Close" @click="closeActiveFile" />
            </span>
          </div>
          <div ref="editorContainer" class="codemirror-container"></div>
        </div>
      </main>

      <div v-if="showPdf" class="resizer" @mousedown="startResize('pdf', $event)"></div>

      <aside v-if="showPdf" class="pdf-panel" :style="{ width: pdfWidth + 'px' }">
        <PdfPreview :pdf-url="pdfUrl" :log="compileLog" @close="showPdf = false" />
      </aside>
    </div>
  </div>
</template>

<script setup>
import { useRoute } from "vue-router";
import { ElMessage } from "element-plus";
import { ref, onMounted, onBeforeUnmount, nextTick } from "vue";
import { ArrowLeft, Upload, RefreshRight, Close, EditPen } from "@element-plus/icons-vue";

import { projectAPI, fileAPI } from "../api";
import FileTree from "../components/FileTree.vue";
import PdfPreview from "../components/PdfPreview.vue";

defineOptions({
  name: "EditorView",
});

let EditorView, EditorState, basicSetup, oneDark;

const route = useRoute();
const projectId = ref(Number(route.params.id));
const project = ref(null);
const fileTree = ref([]);
const activeFile = ref(null);
const activeFileContent = ref("");
const compiling = ref(false);
const showPdf = ref(false);
const pdfUrl = ref("");
const compileLog = ref("");

const sidebarWidth = ref(240);
const pdfWidth = ref(400);
const editorContainer = ref(null);
let cmView = null;
let autoSaveTimer = null;

// ---- 项目重命名 ----
const renamingProject = ref(false);
const renameProjectName = ref("");
const renameInputRef = ref(null);

function startRenameProject() {
  renameProjectName.value = project.value?.name || "";
  renamingProject.value = true;
  nextTick(() => {
    renameInputRef.value?.focus?.();
    renameInputRef.value?.select?.();
  });
}

async function confirmRenameProject() {
  renamingProject.value = false;
  const newName = renameProjectName.value.trim();
  if (!newName || newName === project.value?.name) return;
  try {
    await projectAPI.update(projectId.value, { name: newName });
    project.value.name = newName;
    ElMessage.success("项目已重命名");
  } catch {
    /* handled */
  }
}

onMounted(async () => {
  await loadProject();
  await refreshTree();
  await initCodeMirror();
});

onBeforeUnmount(() => {
  if (cmView) cmView.destroy();
  if (autoSaveTimer) clearTimeout(autoSaveTimer);
});

async function loadProject() {
  try {
    project.value = await projectAPI.get(projectId.value);
  } catch {
    /* handled */
  }
}

async function refreshTree() {
  try {
    const data = await fileAPI.getTree(projectId.value);
    fileTree.value = data || [];
  } catch {
    /* handled */
  }
}

async function initCodeMirror() {
  const [{ basicSetup: bs }, { EditorView: ev }, { EditorState: es }, { oneDark: od }] = await Promise.all([
    import("codemirror"),
    import("@codemirror/view"),
    import("@codemirror/state"),
    import("@codemirror/theme-one-dark"),
  ]);
  EditorView = ev;
  EditorState = es;
  basicSetup = bs;
  oneDark = od;
}

function createEditor() {
  if (!editorContainer.value || !EditorView) return;
  if (cmView) cmView.destroy();

  const updateListener = EditorView.updateListener.of((update) => {
    if (update.docChanged) {
      activeFileContent.value = update.state.doc.toString();
      scheduleAutoSave();
    }
  });

  cmView = new EditorView({
    parent: editorContainer.value,
    state: EditorState.create({
      doc: activeFileContent.value,
      extensions: [basicSetup, oneDark, updateListener, EditorView.lineWrapping],
    }),
  });
}

function scheduleAutoSave() {
  if (autoSaveTimer) clearTimeout(autoSaveTimer);
  autoSaveTimer = setTimeout(() => saveFile(), 2000);
}

async function saveFile() {
  if (!activeFile.value || activeFile.value.is_dir) return;
  try {
    await fileAPI.updateContent(projectId.value, activeFile.value.id, { content: activeFileContent.value });
  } catch {
    /* handled */
  }
}

async function handleFileSelect(file) {
  if (activeFile.value && !activeFile.value.is_dir && activeFile.value.id !== file.id) {
    await saveFile();
  }
  if (file.is_dir) return;
  activeFile.value = file;
  activeFileContent.value = "";
  try {
    const data = await fileAPI.getContent(projectId.value, file.id);
    activeFileContent.value = data.content || "";
  } catch {
    /* handled */
  }
  await nextTick();
  createEditor();
}

function closeActiveFile() {
  activeFile.value = null;
  activeFileContent.value = "";
}

async function handleCreateFile(name, parentId, isDir) {
  try {
    await fileAPI.create(projectId.value, { is_dir: isDir, name, parent_id: parentId });
    await refreshTree();
  } catch {
    /* handled */
  }
}

async function handleRenameFile(fileId, newName) {
  try {
    await fileAPI.rename(projectId.value, fileId, { name: newName });
    if (activeFile.value?.id === fileId) activeFile.value.name = newName;
    await refreshTree();
  } catch {
    /* handled */
  }
}

async function handleDeleteFile(fileId) {
  try {
    await fileAPI.delete(projectId.value, fileId);
    if (activeFile.value?.id === fileId) {
      activeFile.value = null;
      activeFileContent.value = "";
    }
    await refreshTree();
  } catch {
    /* handled */
  }
}

async function handleCompile() {
  await saveFile();
  compiling.value = true;
  compileLog.value = "";
  try {
    const response = await fileAPI.compile(projectId.value);
    if (pdfUrl.value) URL.revokeObjectURL(pdfUrl.value);
    pdfUrl.value = URL.createObjectURL(response.data);
    showPdf.value = true;
    compileLog.value = response.log || "编译成功";
    ElMessage.success("编译完成");
  } catch (err) {
    compileLog.value = err?.message || String(err || "编译失败");
    showPdf.value = true;
  } finally {
    compiling.value = false;
  }
}

function startResize(panel, e) {
  e.preventDefault();
  const startX = e.clientX;
  const startWidth = panel === "sidebar" ? sidebarWidth.value : pdfWidth.value;
  function onMouseMove(ev) {
    const delta = ev.clientX - startX;
    const newWidth =
      panel === "sidebar"
        ? Math.max(180, Math.min(500, startWidth + delta))
        : Math.max(280, Math.min(700, startWidth - delta));
    if (panel === "sidebar") sidebarWidth.value = newWidth;
    else pdfWidth.value = newWidth;
  }
  function onMouseUp() {
    document.removeEventListener("mousemove", onMouseMove);
    document.removeEventListener("mouseup", onMouseUp);
  }
  document.addEventListener("mousemove", onMouseMove);
  document.addEventListener("mouseup", onMouseUp);
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.editor-page {
  @include full-page;
  background: $color-editor-bg;
}

.editor-toolbar {
  height: 40px;
  @include flex-between;
  padding: 0 12px;
  background: $color-panel-bg;
  border-bottom: 1px solid $color-border-dark;
  flex-shrink: 0;
}

.project-title-bar {
  @include flex-between;
  gap: 6px;
}

.project-title {
  font-size: 13px;
  font-weight: 500;
  color: $color-text-dark;
  @include text-ellipsis;
  max-width: 260px;
}

.project-rename-input {
  width: 200px;
}

.toolbar-actions {
  display: flex;
  align-items: center;
  gap: 8px;
}

.editor-body {
  flex: 1;
  display: flex;
  overflow: hidden;
  background: $color-editor-bg;
}

.sidebar {
  background: $color-sidebar;
  border-right: 1px solid $color-border-dark;
  flex-shrink: 0;
  overflow-y: auto;
}

.resizer {
  width: 4px;
  background: $color-border-dark;
  cursor: col-resize;
  flex-shrink: 0;
  transition: background 0.2s;
  &:hover {
    background: $color-accent;
  }
}

.editor-main {
  flex: 1;
  @include flex-column;
  overflow: hidden;
  background: $color-editor-bg;
}

.editor-empty {
  flex: 1;
  @include flex-center;
}

.editor-wrapper {
  flex: 1;
  @include flex-column;
  overflow: hidden;
}

.editor-tab-bar {
  height: $toolbar-height;
  display: flex;
  align-items: stretch;
  background: $color-panel-bg;
  border-bottom: 1px solid $color-border-dark;
  flex-shrink: 0;
  padding: 0 8px;
  gap: 2px;
}

.editor-tab {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 0 12px;
  font-size: 13px;
  color: $color-text-dark;
  background: $color-tab-active;
  border-top: 1px solid $color-accent;
  margin-top: -1px;
  user-select: none;

  .tab-icon {
    font-size: 12px;
  }

  .tab-close {
    margin-left: 4px;
    opacity: 0;
    transition: opacity 0.15s;
  }

  &:hover .tab-close {
    opacity: 1;
  }
}

.codemirror-container {
  flex: 1;
  overflow: auto;
  :deep(.cm-editor) {
    height: 100%;
  }
  :deep(.cm-scroller) {
    overflow: auto;
  }
}

.pdf-panel {
  background: #fff;
  border-left: 1px solid $color-border-dark;
  flex-shrink: 0;
}
</style>

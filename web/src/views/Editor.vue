<template>
  <BaseLayout dark>
    <template #header-left>
      <el-button text :icon="ArrowLeft" @click="$router.push('/')">返回</el-button>
      <span class="project-name">{{ project?.name || "加载中..." }}</span>
    </template>

    <template #header-right>
      <el-button type="primary" :icon="Upload" :loading="compiling" @click="handleCompile">
        {{ compiling ? "编译中..." : "编译" }}
      </el-button>
      <el-button :icon="RefreshRight" @click="refreshTree">刷新</el-button>
    </template>

    <!-- 主体三栏布局 -->
    <div class="editor-body">
      <!-- 左侧: 文件树 -->
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

      <!-- 分隔条 -->
      <div class="resizer" @mousedown="startResize('sidebar', $event)"></div>

      <!-- 中间: 编辑器 -->
      <main class="editor-main">
        <div v-if="!activeFile" class="editor-empty">
          <el-empty description="从左侧文件树选择一个文件开始编辑" />
        </div>
        <div v-else class="editor-wrapper">
          <div class="editor-tab">
            <span>{{ activeFile.name }}</span>
            <el-button
              text
              size="small"
              :icon="Close"
              @click="
                activeFile = null;
                activeFileContent = '';
              "
            />
          </div>
          <div ref="editorContainer" class="codemirror-container"></div>
        </div>
      </main>

      <!-- 分隔条 -->
      <div v-if="showPdf" class="resizer" @mousedown="startResize('pdf', $event)"></div>

      <!-- 右侧: PDF 预览 -->
      <aside v-if="showPdf" class="pdf-panel" :style="{ width: pdfWidth + 'px' }">
        <PdfPreview :pdf-url="pdfUrl" :log="compileLog" @close="showPdf = false" />
      </aside>
    </div>
  </BaseLayout>
</template>

<script setup>
import { ArrowLeft, Upload, RefreshRight, Close } from "@element-plus/icons-vue";
import { ElMessage } from "element-plus";
import { ref, onMounted, onBeforeUnmount, nextTick } from "vue";
import { useRoute } from "vue-router";
import { projectAPI, fileAPI } from "../api";
import FileTree from "../components/FileTree.vue";
import PdfPreview from "../components/PdfPreview.vue";
import BaseLayout from "../layouts/BaseLayout.vue";

// ---- CodeMirror 动态导入 (减少初始加载体积) ----
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

// 面板尺寸
const sidebarWidth = ref(240);
const pdfWidth = ref(400);
const editorContainer = ref(null);
let cmView = null;

// 自动保存定时器
let autoSaveTimer = null;

onMounted(async () => {
  await loadProject();
  await refreshTree();
  await initCodeMirror();
});

onBeforeUnmount(() => {
  if (cmView) cmView.destroy();
  if (autoSaveTimer) clearTimeout(autoSaveTimer);
});

// ---- 数据加载 ----

async function loadProject() {
  try {
    const { data } = await projectAPI.get(projectId.value);
    project.value = data;
  } catch {
    /* handled */
  }
}

async function refreshTree() {
  try {
    const { data } = await fileAPI.getTree(projectId.value);
    fileTree.value = data || [];
  } catch {
    /* handled */
  }
}

// ---- CodeMirror 初始化 ----

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
    state: EditorState.create({
      doc: activeFileContent.value,
      extensions: [basicSetup, oneDark, updateListener, EditorView.lineWrapping],
    }),
    parent: editorContainer.value,
  });
}

function scheduleAutoSave() {
  if (autoSaveTimer) clearTimeout(autoSaveTimer);
  autoSaveTimer = setTimeout(() => {
    saveFile();
  }, 2000); // 2 秒防抖
}

async function saveFile() {
  if (!activeFile.value || activeFile.value.is_dir) return;
  try {
    await fileAPI.updateContent(projectId.value, activeFile.value.id, {
      content: activeFileContent.value,
    });
  } catch {
    /* handled */
  }
}

// ---- 文件操作 ----

async function handleFileSelect(file) {
  // 先保存当前文件
  if (activeFile.value && !activeFile.value.is_dir && activeFile.value.id !== file.id) {
    await saveFile();
  }

  if (file.is_dir) return;

  activeFile.value = file;
  activeFileContent.value = "";

  // 加载文件内容
  try {
    const { data } = await fileAPI.getContent(projectId.value, file.id);
    activeFileContent.value = data.content || "";
  } catch {
    /* handled */
  }

  await nextTick();
  createEditor();
}

async function handleCreateFile(name, parentId, isDir) {
  try {
    await fileAPI.create(projectId.value, {
      name,
      parent_id: parentId,
      is_dir: isDir,
    });
    await refreshTree();
  } catch {
    /* handled */
  }
}

async function handleRenameFile(fileId, newName) {
  try {
    await fileAPI.rename(projectId.value, fileId, { name: newName });
    if (activeFile.value?.id === fileId) {
      activeFile.value.name = newName;
    }
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

// ---- 编译 ----

async function handleCompile() {
  // 先保存当前文件
  await saveFile();

  compiling.value = true;
  compileLog.value = "";
  try {
    const response = await fileAPI.compile(projectId.value);
    // 创建 blob URL 用于 PDF 预览
    if (pdfUrl.value) URL.revokeObjectURL(pdfUrl.value);
    pdfUrl.value = URL.createObjectURL(response.data);
    showPdf.value = true;
    compileLog.value = "编译成功";
    ElMessage.success("编译完成");
  } catch (err) {
    // 尝试从错误响应中提取编译日志
    const errorData = err.response?.data;
    if (errorData instanceof Blob) {
      const text = await errorData.text();
      try {
        const json = JSON.parse(text);
        compileLog.value = json.log || json.error || text;
      } catch {
        compileLog.value = text;
      }
    }
    showPdf.value = true; // 即使失败也显示日志面板
  } finally {
    compiling.value = false;
  }
}

// ---- 面板拖拽调整大小 ----

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

<style scoped>
.editor-body {
  flex: 1;
  display: flex;
  overflow: hidden;
  background: var(--color-bg-dark);
  color: #d4d4d4;
}

.project-name {
  font-size: 14px;
  font-weight: 500;
}

.sidebar {
  background: var(--color-sidebar);
  border-right: 1px solid var(--color-border-dark);
  flex-shrink: 0;
  overflow-y: auto;
}

.resizer {
  width: 4px;
  background: var(--color-border-dark);
  cursor: col-resize;
  flex-shrink: 0;
  transition: background 0.2s;
}

.resizer:hover {
  background: var(--color-primary);
}

.editor-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.editor-empty {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  background: var(--color-bg-dark);
}

.editor-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.editor-tab {
  height: 35px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 12px;
  background: #2d2d2d;
  border-bottom: 1px solid var(--color-border-dark);
  font-size: 13px;
  flex-shrink: 0;
}

.codemirror-container {
  flex: 1;
  overflow: auto;
}

/* 覆盖 CodeMirror 默认高度 */
.codemirror-container :deep(.cm-editor) {
  height: 100%;
}

.codemirror-container :deep(.cm-scroller) {
  overflow: auto;
}

.pdf-panel {
  background: #fff;
  border-left: 1px solid var(--color-border-dark);
  flex-shrink: 0;
}
</style>

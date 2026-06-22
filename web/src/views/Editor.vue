<template>
  <div class="editor-page">
    <div class="editor-toolbar">
      <div class="toolbar-left">
        <el-button text class="back-button" :icon="ArrowLeft" @click="$router.push('/')">返回</el-button>
        <span class="toolbar-divider" />
        <div class="panel-switcher">
          <el-button
            v-for="tab in panelTabs"
            :key="tab.key"
            text
            size="small"
            class="switcher-btn"
            :class="{ active: activePanel === tab.key }"
            @click="setPanel(tab.key)"
          >
            {{ tab.label }}
          </el-button>
        </div>
      </div>

      <div class="toolbar-center">
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
          <el-button
            v-if="!renamingProject"
            text
            size="small"
            class="rename-button"
            :icon="EditPen"
            @click="startRenameProject"
          />
        </div>
      </div>

      <div class="toolbar-actions">
        <el-tooltip content="刷新文件树" placement="bottom">
          <el-button class="icon-action" :icon="RefreshRight" @click="refreshTree" />
        </el-tooltip>
        <el-tooltip content="在文件夹中打开项目" placement="bottom">
          <el-button class="icon-action" :icon="FolderOpened" @click="handleOpenFolder" />
        </el-tooltip>
        <el-tooltip content="定位到 PDF（Synctex）" placement="bottom">
          <el-button class="icon-action" :icon="Aim" :disabled="!showPdf" @click="handleSynctexForward" />
        </el-tooltip>
        <el-button type="primary" class="compile-button" :icon="Upload" :loading="compiling" @click="handleCompile">
          {{ compiling ? "编译中..." : "编译 PDF" }}
        </el-button>
      </div>
    </div>

    <div class="editor-body">
      <aside class="sidebar" :style="{ width: sidebarWidth + 'px' }">
        <FileTree
          v-show="activePanel === 'files'"
          :files="fileTree"
          :active-file-id="activeFile?.id"
          @select="handleFileSelect"
          @create-file="handleCreateFile"
          @rename="handleRenameFile"
          @delete="handleDeleteFile"
        />
        <HistoryPanel v-if="activePanel === 'history'" :project-id="projectId" :refresh-key="panelRefreshKey" />
        <SnapshotPanel v-if="activePanel === 'snapshots'" :project-id="projectId" :refresh-key="panelRefreshKey" />
      </aside>

      <div class="resizer" @mousedown="startResize('sidebar', $event)"></div>

      <main class="editor-main">
        <div v-if="!activeFile" class="editor-empty">
          <el-icon :size="42"><Document /></el-icon>
          <p>未打开文件</p>
        </div>
        <div v-else class="editor-wrapper">
          <div class="editor-tab-bar">
            <span class="editor-tab active">
              <el-icon :size="14" class="tab-icon"><Document /></el-icon>
              {{ activeFile.name }}
              <el-button text size="small" class="tab-close" :icon="Close" @click="closeActiveFile" />
            </span>
          </div>
          <div ref="editorContainer" class="codemirror-container"></div>
          <div class="editor-statusbar">
            <span>{{ activeFile.name }}</span>
            <span>LaTeX</span>
          </div>
        </div>
      </main>

      <div v-if="showPdf" class="resizer" @mousedown="startResize('pdf', $event)"></div>

      <aside v-if="showPdf" class="pdf-panel" :style="{ width: pdfWidth + 'px' }">
        <PdfPreview
          :pdf-blob-url="pdfUrl"
          :log="compileLog"
          :synctex-target="synctexTarget"
          @close="showPdf = false"
          @inverse="handleSynctexInverse"
        />
      </aside>
    </div>
  </div>
</template>

<script setup>
import { useRoute } from "vue-router";
import { ElMessage } from "element-plus";
import { ref, onMounted, onBeforeUnmount, nextTick, watch } from "vue";
import { ArrowLeft, Aim, Close, Document, EditPen, FolderOpened, RefreshRight, Upload } from "@element-plus/icons-vue";

import { projectAPI, fileAPI } from "../api";
import { useThemeStore } from "../stores/theme";
import FileTree from "../components/FileTree.vue";
import PdfPreview from "../components/PdfPreview.vue";
import HistoryPanel from "../components/HistoryPanel.vue";
import SnapshotPanel from "../components/SnapshotPanel.vue";

defineOptions({
  name: "EditorView",
});

let EditorView, EditorState, Prec, basicSetup, oneDark;

const route = useRoute();
const themeStore = useThemeStore();
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

// ---- SyncTeX ----
const synctexTarget = ref(null); // { page, v } 触发 PdfPreview 滚动

// ---- 侧面板切换 ----
const activePanel = ref("files");
const panelRefreshKey = ref(0);
const panelTabs = [
  { key: "files", label: "文件" },
  { key: "history", label: "历史" },
  { key: "snapshots", label: "快照" },
];

function setPanel(key) {
  if (activePanel.value === key) return;
  activePanel.value = key;
  panelRefreshKey.value += 1; // 切换后刷新面板数据
}

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

onMounted(loadCurrentProject);

onBeforeUnmount(() => {
  if (cmView) cmView.destroy();
  if (autoSaveTimer) clearTimeout(autoSaveTimer);
});

watch(
  () => themeStore.editorTheme,
  () => {
    if (!cmView) return;
    activeFileContent.value = cmView.state.doc.toString();
    createEditor();
  },
);

watch(
  () => route.params.id,
  async () => {
    await loadCurrentProject();
  },
);

async function loadCurrentProject() {
  projectId.value = Number(route.params.id);
  resetEditorState();
  await loadProject();
  await refreshTree();
  await initCodeMirror();
  await openInitialFile();
}

function resetEditorState() {
  if (cmView) {
    cmView.destroy();
    cmView = null;
  }
  if (autoSaveTimer) {
    clearTimeout(autoSaveTimer);
    autoSaveTimer = null;
  }
  project.value = null;
  fileTree.value = [];
  activeFile.value = null;
  activeFileContent.value = "";
  if (pdfUrl.value) URL.revokeObjectURL(pdfUrl.value);
  pdfUrl.value = "";
  showPdf.value = false;
  compileLog.value = "";
}

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

async function openInitialFile() {
  const targetName = Array.isArray(route.query.open) ? route.query.open[0] : route.query.open;
  if (!targetName) return;
  const target = findFileByName(fileTree.value, String(targetName));
  if (target) await handleFileSelect(target);
}

function findFileByName(files, name) {
  for (const file of files) {
    if (!file.is_dir && file.name === name) return file;
    const child = findFileByName(file.children || [], name);
    if (child) return child;
  }
  return null;
}

async function initCodeMirror() {
  if (EditorView) return;
  const [{ basicSetup: bs }, { EditorView: ev }, stateModule, { oneDark: od }] = await Promise.all([
    import("codemirror"),
    import("@codemirror/view"),
    import("@codemirror/state"),
    import("@codemirror/theme-one-dark"),
  ]);
  EditorView = ev;
  EditorState = stateModule.EditorState;
  Prec = stateModule.Prec;
  basicSetup = bs;
  oneDark = od;
}

function buildCodeMirrorTheme(isDark) {
  return Prec.high(
    EditorView.theme(
      {
        ".cm-activeLine": {
          backgroundColor: isDark ? "rgba(255, 255, 255, 0.035)" : "rgba(47, 111, 78, 0.06)",
        },
        ".cm-activeLineGutter": {
          backgroundColor: "var(--editor-accent-soft)",
          color: "var(--editor-accent)",
        },
        ".cm-content": {
          caretColor: "var(--editor-caret)",
          minHeight: "100%",
          padding: "18px 0 64px",
        },
        ".cm-gutters": {
          backgroundColor: "var(--editor-gutter-bg)",
          borderRight: "1px solid var(--editor-border)",
          color: "var(--editor-line-number)",
        },
        ".cm-line": {
          padding: "0 24px",
        },
        ".cm-scroller": {
          fontFamily: '"Cascadia Code", "JetBrains Mono", Consolas, "Courier New", monospace',
          lineHeight: "1.65",
        },
        "&": {
          backgroundColor: "var(--editor-code-bg)",
          color: "var(--editor-text)",
          fontSize: "14px",
          height: "100%",
        },
        "&.cm-focused": {
          outline: "none",
        },
        "&.cm-focused .cm-cursor": {
          borderLeftColor: "var(--editor-caret)",
        },
        "&.cm-focused .cm-selectionBackground, .cm-selectionBackground, .cm-content ::selection": {
          backgroundColor: "var(--editor-selection)",
        },
      },
      { dark: isDark },
    ),
  );
}

function createEditor() {
  if (!editorContainer.value || !EditorView) return;
  if (cmView) cmView.destroy();
  const isDark = themeStore.editorTheme === "dark";

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
      extensions: [
        basicSetup,
        ...(isDark ? [oneDark] : []),
        buildCodeMirrorTheme(isDark),
        updateListener,
        EditorView.lineWrapping,
      ],
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
    panelRefreshKey.value += 1; // 删除会生成快照，刷新历史/快照面板
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

async function handleOpenFolder() {
  try {
    await fileAPI.openProjectFolder(projectId.value);
  } catch {
    /* handled */
  }
}

// ---- SyncTeX 正反向同步 ----

// 正向同步：当前光标位置 → PDF 对应区域
async function handleSynctexForward() {
  if (!cmView || !activeFile.value || !showPdf.value) return;
  try {
    const pos = cmView.state.selection.main.head;
    const line = cmView.state.doc.lineAt(pos).number;
    // synctex 期望的 input 是 TeX 认识的文件名，这里用文件的相对路径（根目录下即文件名）
    const input = activeFile.value.name;
    const result = await fileAPI.synctexForward(projectId.value, input, line, 0);
    if (result && result.page > 0) {
      // 触发 PdfPreview 滚动：每次都创建新对象让 watch 捕获
      synctexTarget.value = { page: result.page, ts: Date.now(), v: result.v || result.y };
    }
  } catch (err) {
    ElMessage.warning(err?.message || "无法定位到 PDF，请确认已开启 synctex 编译");
  }
}

// 反向同步：PDF 点击 → 跳转编辑器光标
async function handleSynctexInverse({ page, x, y }) {
  try {
    const result = await fileAPI.synctexInverse(projectId.value, page, x, y);
    if (!result || !result.line) return;
    // 查找对应文件并打开，然后定位到行
    const fileName = extractFileName(result.input);
    await ensureFileOpenByName(fileName);
    jumpToLine(result.line, Math.max(0, result.column));
  } catch (err) {
    ElMessage.warning(err?.message || "无法定位到源码");
  }
}

// 从 synctex 返回的绝对/相对路径提取文件名
function extractFileName(inputPath) {
  if (!inputPath) return "";
  const normalized = inputPath.replace(/\\/g, "/");
  const parts = normalized.split("/");
  return parts[parts.length - 1] || "";
}

// 确保某个文件名的文件已打开（用于反向同步跳转）
async function ensureFileOpenByName(fileName) {
  if (!fileName) return;
  if (activeFile.value && activeFile.value.name === fileName) return;
  const found = findFileByName(fileTree.value, fileName);
  if (found) await handleFileSelect(found);
}

// 跳转到指定行列
function jumpToLine(line, column) {
  if (!cmView) return;
  const doc = cmView.state.doc;
  if (line < 1 || line > doc.lines) return;
  const lineObj = doc.line(line);
  const col = Math.min(column || 0, Math.max(0, lineObj.length));
  const pos = lineObj.from + col;
  cmView.dispatch({
    scrollIntoView: true,
    selection: { anchor: pos, head: pos },
  });
  cmView.focus();
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
  background: var(--editor-bg);
}

.editor-toolbar {
  height: 46px;
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 0 12px;
  background: var(--editor-panel-bg);
  border-bottom: 1px solid var(--editor-border);
  flex-shrink: 0;
}

.toolbar-left {
  // 左边区域：固定宽度，容纳「返回」按钮宽度的约 3.5 倍（返回 + 竖线 + 3 个切换 tab）
  display: flex;
  align-items: center;
  gap: 8px;
  flex: 0 0 auto;
  width: 280px;
}

.toolbar-divider {
  width: 1px;
  height: 20px;
  background: var(--editor-border);
  flex-shrink: 0;
}

.panel-switcher {
  display: flex;
  align-items: center;
  gap: 2px;
}

.switcher-btn {
  color: var(--editor-text-dim);
  font-weight: 500;
  padding: 0 8px;

  &:hover,
  &:focus {
    background: var(--editor-hover-bg);
    color: var(--editor-text);
  }

  &.active {
    color: var(--editor-accent);
    font-weight: 600;
  }
}

.toolbar-center {
  // 中间区域：项目名居中，两侧带竖线分隔
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  min-width: 0;
  padding: 0 16px;
  border-left: 1px solid var(--editor-border);
  border-right: 1px solid var(--editor-border);
  height: 100%;
}

.toolbar-actions {
  // 右边区域：固定宽度，约「返回」按钮宽的 4.5 倍
  display: flex;
  align-items: center;
  gap: 8px;
  flex: 0 0 auto;
  width: 360px;
  justify-content: flex-end;
}

.back-button,
.rename-button {
  color: var(--editor-text-dim);

  &:hover,
  &:focus {
    background: var(--editor-hover-bg);
    color: var(--editor-text);
  }
}

.project-title-bar {
  display: flex;
  align-items: center;
  min-width: 0;
  gap: 4px;
}

.project-title {
  font-size: 14px;
  font-weight: 650;
  color: var(--editor-text);
  @include text-ellipsis;
  max-width: 320px;
  cursor: default;
}

.project-rename-input {
  width: 200px;
}

.icon-action {
  width: 34px;
  height: 34px;
  padding: 0;
  border-color: var(--editor-border);
  background: var(--editor-panel-bg);
  color: var(--editor-text-dim);

  &:hover,
  &:focus {
    border-color: var(--editor-accent);
    background: var(--editor-hover-bg);
    color: var(--editor-text);
  }
}

.compile-button {
  height: 34px;
  border: none;
  border-radius: 8px;
  background: var(--editor-accent);
  color: var(--app-primary-contrast);
  font-weight: 600;

  &:hover,
  &:focus {
    background: var(--app-primary-hover);
    color: var(--app-primary-contrast);
  }
}

.editor-body {
  flex: 1;
  display: flex;
  min-height: 0;
  margin: 12px;
  border: 1px solid var(--editor-border);
  border-radius: 8px;
  overflow: hidden;
  background: var(--editor-bg);
  box-shadow: var(--app-shadow);
}

.sidebar {
  background: var(--editor-sidebar-bg);
  border-right: 1px solid var(--editor-border);
  flex-shrink: 0;
  overflow-y: auto;
}

.resizer {
  width: 5px;
  background: var(--editor-bg);
  cursor: col-resize;
  flex-shrink: 0;
  transition: background 0.2s;
  &:hover {
    background: var(--editor-accent);
  }
}

.editor-main {
  flex: 1;
  @include flex-column;
  overflow: hidden;
  background: var(--editor-bg);
}

.editor-empty {
  flex: 1;
  @include flex-center;
  flex-direction: column;
  gap: 10px;
  color: var(--editor-text-dim);

  p {
    color: var(--editor-text);
    font-size: 15px;
    font-weight: 650;
  }
}

.editor-wrapper {
  flex: 1;
  @include flex-column;
  overflow: hidden;
}

.editor-tab-bar {
  height: 38px;
  display: flex;
  align-items: stretch;
  background: var(--editor-panel-bg);
  border-bottom: 1px solid var(--editor-border);
  flex-shrink: 0;
  padding: 0 8px;
  gap: 2px;
}

.editor-tab {
  display: flex;
  align-items: center;
  gap: 6px;
  min-width: 132px;
  max-width: 260px;
  padding: 0 10px 0 12px;
  font-size: 13px;
  font-weight: 550;
  color: var(--editor-text);
  background: var(--editor-tab-active);
  border: 1px solid var(--editor-border);
  border-bottom-color: var(--editor-tab-active);
  border-radius: 7px 7px 0 0;
  margin-top: 5px;
  user-select: none;
  @include text-ellipsis;

  .tab-icon {
    color: var(--editor-text-dim);
    flex-shrink: 0;
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
  min-height: 0;
  overflow: auto;
  background: var(--editor-code-bg);
  :deep(.cm-editor) {
    height: 100%;
  }
  :deep(.cm-scroller) {
    overflow: auto;
  }
}

.editor-statusbar {
  height: 26px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 10px;
  padding: 0 12px;
  border-top: 1px solid var(--editor-border);
  background: var(--editor-panel-bg);
  color: var(--editor-text-dim);
  flex-shrink: 0;
  font-size: 12px;
}

.pdf-panel {
  background: var(--editor-panel-bg);
  border-left: 1px solid var(--editor-border);
  flex-shrink: 0;
}
</style>

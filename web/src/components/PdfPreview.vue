<template>
  <div class="pdf-preview">
    <div class="pdf-header">
      <span>预览</span>
      <div class="pdf-tools">
        <span v-if="pageCount > 0" class="page-indicator">{{ currentPage }} / {{ pageCount }}</span>
        <el-tooltip content="缩小" placement="bottom">
          <el-button text size="small" :icon="ZoomOut" :disabled="!pdf || scale <= 0.5" @click="zoomOut" />
        </el-tooltip>
        <el-tooltip content="放大" placement="bottom">
          <el-button text size="small" :icon="ZoomIn" :disabled="!pdf || scale >= 3" @click="zoomIn" />
        </el-tooltip>
        <el-button text size="small" :icon="Close" @click="$emit('close')" />
      </div>
    </div>
    <div class="pdf-content">
      <!-- 编译日志（无 PDF 时） -->
      <div v-if="log && !pdfBlobUrl" class="compile-log">
        <div class="log-title">编译日志</div>
        <pre class="log-text">{{ log }}</pre>
      </div>
      <!-- PDF 渲染 -->
      <div v-show="pdfBlobUrl" ref="viewerRef" class="pdf-viewer" @scroll="onScroll">
        <div v-if="loading" class="pdf-loading">渲染中...</div>
        <div
          v-for="pageInfo in renderedPages"
          :key="pageInfo.pageNumber"
          :ref="(el) => setPageRef(el, pageInfo.pageNumber)"
          class="pdf-page-wrapper"
        >
          <canvas
            class="pdf-canvas"
            @click="onPageClick($event, pageInfo.pageNumber)"
            @dblclick="onPageClick($event, pageInfo.pageNumber)"
          />
          <div class="pdf-page-label">{{ pageInfo.pageNumber }}</div>
        </div>
      </div>
      <!-- 空状态 -->
      <div v-if="!pdfBlobUrl && !log" class="pdf-empty">
        <el-icon :size="48"><PictureFilled /></el-icon>
        <p>点击上方「编译」按钮生成 PDF 预览</p>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, watch, onBeforeUnmount, nextTick } from "vue";
import { Close, PictureFilled, ZoomIn, ZoomOut } from "@element-plus/icons-vue";

const props = defineProps({
  // PDF 的 Blob URL，由父组件编译后生成
  log: { default: "", type: String },
  pdfBlobUrl: { default: "", type: String },
  // 正向同步目标：{ page, v } （v 是 synctex 的 big-point 坐标）
  synctexTarget: { default: null, type: Object },
});

const emit = defineEmits(["close", "inverse", "forward-done"]);

// pdf.js 运行时
let pdfjsLib = null;
let pdf = null; // PDFDocumentProxy

const viewerRef = ref(null);
const loading = ref(false);
const pageCount = ref(0);
const currentPage = ref(1);
const scale = ref(1.2);

// 每页的 viewport 尺寸，用于占位 + 坐标转换
// key = pageNumber, value = { width, height }（CSS 像素）
const pageViewports = ref({});
const renderedPages = ref([]); // [{ pageNumber, width, height }]

// canvas DOM 引用
const pageRefs = new Map();
function setPageRef(el, pageNumber) {
  if (el) pageRefs.set(pageNumber, el);
  else pageRefs.delete(pageNumber);
}

// ---- 生命周期 ----
onBeforeUnmount(() => {
  cleanupPdf();
});

watch(
  () => props.pdfBlobUrl,
  (url) => {
    if (url) loadPdf(url);
    else cleanupPdf();
  },
);

watch(scale, async () => {
  if (pdf) await renderAll();
});

// 正向同步：滚动到目标位置
watch(
  () => props.synctexTarget,
  async (target) => {
    if (!target || !pdf) return;
    await scrollToSynctex(target.page, target.v);
  },
);

async function ensurePdfjs() {
  if (pdfjsLib) return;
  const mod = await import("pdfjs-dist");
  pdfjsLib = mod;
  // worker 配置：使用打包后的 worker
  if (mod.GlobalWorkerOptions) {
    mod.GlobalWorkerOptions.workerSrc = new URL("pdfjs-dist/build/pdf.worker.min.mjs", import.meta.url).toString();
  }
}

async function loadPdf(url) {
  loading.value = true;
  try {
    await ensurePdfjs();
    const task = pdfjsLib.getDocument(url);
    pdf = await task.promise;
    pageCount.value = pdf.numPages;
    renderedPages.value = [];
    pageViewports.value = {};
    await renderAll();
  } catch (err) {
    console.error("PDF 加载失败", err);
  } finally {
    loading.value = false;
  }
}

async function renderAll() {
  if (!pdf) return;
  const pages = [];
  for (let i = 1; i <= pdf.numPages; i += 1) {
    const vp = await pdf.getPage(i);
    const viewport = vp.getViewport({ scale: scale.value });
    pageViewports.value[i] = { height: viewport.height, width: viewport.width };
    pages.push({ height: viewport.height, pageNumber: i, width: viewport.width });
  }
  renderedPages.value = pages;
  // 等 DOM 更新后真正绘制 canvas
  await nextTick();
  for (let i = 1; i <= pdf.numPages; i += 1) {
    await renderPageCanvas(i);
  }
}

async function renderPageCanvas(pageNumber) {
  if (!pdf) return;
  const wrapper = pageRefs.get(pageNumber);
  if (!wrapper) return;
  const canvas = wrapper.querySelector("canvas");
  if (!canvas) return;
  const page = await pdf.getPage(pageNumber);
  const viewport = page.getViewport({ scale: scale.value });
  const dpr = window.devicePixelRatio || 1;
  canvas.width = Math.floor(viewport.width * dpr);
  canvas.height = Math.floor(viewport.height * dpr);
  canvas.style.width = `${viewport.width}px`;
  canvas.style.height = `${viewport.height}px`;
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  await page.render({ canvasContext: ctx, viewport }).promise;
}

// ---- 缩放 ----
function zoomIn() {
  scale.value = Math.min(3, Math.round((scale.value + 0.2) * 10) / 10);
}
function zoomOut() {
  scale.value = Math.max(0.5, Math.round((scale.value - 0.2) * 10) / 10);
}

// ---- 滚动 ----
function onScroll() {
  if (!viewerRef.value) return;
  const container = viewerRef.value;
  // 找当前可见的页
  let best = 1;
  let bestDist = Infinity;
  for (let i = 1; i <= pageCount.value; i += 1) {
    const el = pageRefs.get(i);
    if (!el) continue;
    const rect = el.getBoundingClientRect();
    const cRect = container.getBoundingClientRect();
    const dist = Math.abs(rect.top - cRect.top);
    if (rect.bottom > cRect.top && dist < bestDist) {
      best = i;
      bestDist = dist;
    }
  }
  currentPage.value = best;
}

// ---- 反向同步：PDF 点击 → 源码 ----
// synctex 坐标单位是 big point (72 dpi)，pdf.js 渲染是 CSS px (96 dpi)。
// 1 bp = 96/72 css px，故 css px → bp 要乘 72/96。
const BP_PER_CSS_PX = 72 / 96;

function onPageClick(event, pageNumber) {
  const canvas = event.currentTarget;
  const rect = canvas.getBoundingClientRect();
  // 点击点相对页面左上角的 CSS 像素
  const cssX = event.clientX - rect.left;
  const cssY = event.clientY - rect.top;
  // 转换为 big point（synctex 期望的坐标）
  const bpX = (cssX / scale.value) * BP_PER_CSS_PX;
  const bpY = (cssY / scale.value) * BP_PER_CSS_PX;
  emit("inverse", { page: pageNumber, x: bpX, y: bpY });
}

// ---- 正向同步：滚动到 synctex 返回的位置 ----
async function scrollToSynctex(page, v) {
  if (!viewerRef.value) return;
  await nextTick();
  const wrapper = pageRefs.get(page);
  if (!wrapper) {
    // 页码超出范围，滚动到底部
    return;
  }
  const container = viewerRef.value;
  const containerRect = container.getBoundingClientRect();
  const wrapperRect = wrapper.getBoundingClientRect();
  // v 是 big point（页面顶部到目标的距离），转 CSS px
  const targetCssY = (v / BP_PER_CSS_PX) * scale.value;
  // wrapper 当前相对 container 的偏移 + 目标在页内的偏移
  const offsetWithinContainer = wrapperRect.top - containerRect.top + container.scrollTop;
  const targetScrollTop = offsetWithinContainer + targetCssY - container.clientHeight / 2;
  container.scrollTo({ behavior: "smooth", top: Math.max(0, targetScrollTop) });
  currentPage.value = page;
  // 高亮闪烁（可选：临时给页面加个标记）
  highlightPage(page, targetCssY);
  emit("forward-done");
}

let highlightEl = null;
function highlightPage(page, targetCssY) {
  if (highlightEl) {
    highlightEl.remove();
    highlightEl = null;
  }
  const wrapper = pageRefs.get(page);
  if (!wrapper) return;
  highlightEl = document.createElement("div");
  highlightEl.className = "synctex-highlight";
  highlightEl.style.top = `${targetCssY - 4}px`;
  wrapper.appendChild(highlightEl);
  setTimeout(() => {
    if (highlightEl) {
      highlightEl.remove();
      highlightEl = null;
    }
  }, 1500);
}

function cleanupPdf() {
  pdf = null;
  pageCount.value = 0;
  currentPage.value = 1;
  renderedPages.value = [];
  pageViewports.value = {};
  pageRefs.clear();
  if (highlightEl) {
    highlightEl.remove();
    highlightEl = null;
  }
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.pdf-preview {
  height: 100%;
  @include flex-column;
  background: var(--editor-panel-bg);
  color: var(--editor-text);
}

.pdf-header {
  @include panel-header(38px);
  background: var(--editor-panel-bg);
  gap: 8px;

  .pdf-tools {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  .page-indicator {
    font-size: 11px;
    color: var(--editor-text-dim);
    margin-right: 4px;
    min-width: 48px;
    text-align: right;
  }

  :deep(.el-button) {
    color: var(--editor-text-dim);

    &:hover,
    &:focus {
      background: var(--editor-hover-bg);
      color: var(--editor-text);
    }
  }
}

.pdf-content {
  flex: 1;
  overflow: hidden;
  background: var(--editor-code-bg);
  position: relative;
}

.pdf-viewer {
  height: 100%;
  overflow-y: auto;
  padding: 12px;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;

  @include custom-scrollbar;
}

.pdf-loading {
  padding: 16px;
  color: var(--editor-text-dim);
  font-size: 13px;
}

.pdf-page-wrapper {
  position: relative;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.25);
  background: white;
}

.pdf-canvas {
  display: block;
  cursor: crosshair;
}

.pdf-page-label {
  position: absolute;
  bottom: 4px;
  right: 6px;
  font-size: 10px;
  color: rgba(0, 0, 0, 0.4);
  pointer-events: none;
}

:deep(.synctex-highlight) {
  position: absolute;
  left: 0;
  right: 0;
  height: 8px;
  background: var(--editor-accent);
  opacity: 0.35;
  pointer-events: none;
  border-radius: 2px;
  animation: synctex-pulse 1.5s ease-out;
}

@keyframes synctex-pulse {
  0% {
    opacity: 0.6;
  }
  100% {
    opacity: 0;
  }
}

.pdf-empty {
  height: 100%;
  @include flex-center;
  flex-direction: column;
  color: var(--editor-text-dim);
  gap: 12px;
  p {
    font-size: 13px;
  }
}

.compile-log {
  height: 100%;
  @include flex-column;
  padding: 12px;
  overflow-y: auto;
  background: var(--editor-code-bg);
}

.log-title {
  font-size: 13px;
  font-weight: 600;
  margin-bottom: 8px;
  color: var(--editor-text);
}

.log-text {
  flex: 1;
  font-size: 12px;
  font-family: Consolas, "Courier New", monospace;
  white-space: pre-wrap;
  word-break: break-all;
  color: var(--editor-text-dim);
  line-height: 1.5;
}
</style>

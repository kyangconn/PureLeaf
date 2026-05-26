<template>
  <div class="dashboard">
    <!-- 顶部导航 -->
    <header class="dash-header">
      <div class="dash-brand">goleaf</div>
      <div class="dash-user">
        <span class="dash-username">{{ authStore.user?.username }}</span>
        <el-button text @click="handleLogout">退出</el-button>
      </div>
    </header>

    <!-- 内容区 -->
    <main class="dash-main">
      <div class="dash-toolbar">
        <h2>我的项目</h2>
        <el-button type="primary" :icon="Plus" @click="showCreateDialog = true"
          >新建项目</el-button
        >
      </div>

      <!-- 项目列表 -->
      <div v-if="projects.length === 0" class="dash-empty">
        <el-empty
          description="还没有项目，点击上方按钮创建第一个 LaTeX 项目吧"
        />
      </div>

      <div v-else class="project-grid">
        <div
          v-for="project in projects"
          :key="project.id"
          class="project-card"
          @click="$router.push(`/project/${project.id}`)"
        >
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
                  <el-dropdown-item @click="openRename(project)"
                    >重命名</el-dropdown-item
                  >
                  <el-dropdown-item divided @click="handleDelete(project)"
                    >删除</el-dropdown-item
                  >
                </el-dropdown-menu>
              </template>
            </el-dropdown>
          </div>
        </div>
      </div>
    </main>

    <!-- 新建项目对话框 -->
    <el-dialog v-model="showCreateDialog" title="新建项目" width="400px">
      <el-form
        ref="createFormRef"
        :model="createForm"
        :rules="createRules"
        @keyup.enter="handleCreate"
      >
        <el-form-item prop="name">
          <el-input v-model="createForm.name" placeholder="项目名称" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showCreateDialog = false">取消</el-button>
        <el-button type="primary" :loading="creating" @click="handleCreate"
          >创建</el-button
        >
      </template>
    </el-dialog>

    <!-- 重命名对话框 -->
    <el-dialog v-model="showRenameDialog" title="重命名项目" width="400px">
      <el-form
        ref="renameFormRef"
        :model="renameForm"
        :rules="renameRules"
        @keyup.enter="handleRename"
      >
        <el-form-item prop="name">
          <el-input v-model="renameForm.name" placeholder="新项目名称" />
        </el-form-item>
      </el-form>
      <template #footer>
        <el-button @click="showRenameDialog = false">取消</el-button>
        <el-button type="primary" :loading="renaming" @click="handleRename"
          >确认</el-button
        >
      </template>
    </el-dialog>
  </div>
</template>

<script setup>
import { Plus, Document, MoreFilled } from "@element-plus/icons-vue";
import { ElMessageBox } from "element-plus";
import { ref, reactive, onMounted } from "vue";
import { projectAPI } from "../api";
import { useAuthStore } from "../stores/auth";

const authStore = useAuthStore();
const projects = ref([]);

// ---- 新建 ----
const showCreateDialog = ref(false);
const creating = ref(false);
const createFormRef = ref(null);
const createForm = reactive({ name: "" });
const createRules = {
  name: [{ required: true, message: "请输入项目名称", trigger: "blur" }],
};

// ---- 重命名 ----
const showRenameDialog = ref(false);
const renaming = ref(false);
const renameFormRef = ref(null);
const renameForm = reactive({ name: "" });
const renameTarget = ref(null);
const renameRules = {
  name: [{ required: true, message: "请输入新名称", trigger: "blur" }],
};

onMounted(fetchProjects);

async function fetchProjects() {
  try {
    const { data } = await projectAPI.list();
    projects.value = data || [];
  } catch {
    /* handled by interceptor */
  }
}

async function handleCreate() {
  const valid = await createFormRef.value.validate().catch(() => false);
  if (!valid) return;
  creating.value = true;
  try {
    await projectAPI.create({ name: createForm.name });
    showCreateDialog.value = false;
    createForm.name = "";
    await fetchProjects();
  } finally {
    creating.value = false;
  }
}

function openRename(project) {
  renameTarget.value = project;
  renameForm.name = project.name;
  showRenameDialog.value = true;
}

async function handleRename() {
  const valid = await renameFormRef.value.validate().catch(() => false);
  if (!valid) return;
  renaming.value = true;
  try {
    await projectAPI.update(renameTarget.value.id, { name: renameForm.name });
    showRenameDialog.value = false;
    await fetchProjects();
  } finally {
    renaming.value = false;
  }
}

async function handleDelete(project) {
  try {
    await ElMessageBox.confirm(
      `确定要删除项目「${project.name}」吗？此操作不可恢复。`,
      "确认删除",
      {
        type: "warning",
        confirmButtonText: "删除",
        cancelButtonText: "取消",
      },
    );
    await projectAPI.delete(project.id);
    await fetchProjects();
  } catch {
    /* cancelled */
  }
}

function handleLogout() {
  authStore.logout();
  window.location.href = "/login";
}

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
.dashboard {
  height: 100%;
  display: flex;
  flex-direction: column;
  background: #f5f7fa;
}

.dash-header {
  height: 56px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;
  background: #fff;
  border-bottom: 1px solid #e4e7ed;
  flex-shrink: 0;
}

.dash-brand {
  font-size: 20px;
  font-weight: 700;
  color: #667eea;
}

.dash-user {
  display: flex;
  align-items: center;
  gap: 8px;
}

.dash-username {
  font-size: 14px;
  color: #606266;
}

.dash-main {
  flex: 1;
  overflow-y: auto;
  padding: 24px;
  max-width: 960px;
  width: 100%;
  margin: 0 auto;
}

.dash-toolbar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 24px;
}

.dash-toolbar h2 {
  font-size: 20px;
  font-weight: 600;
  color: #303133;
}

.dash-empty {
  margin-top: 80px;
}

.project-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 16px;
}

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
  border: 1px solid #e4e7ed;
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
  background: #ecf5ff;
  border-radius: 8px;
  color: #667eea;
  flex-shrink: 0;
}

.project-info {
  flex: 1;
  min-width: 0;
}

.project-name {
  font-size: 15px;
  font-weight: 500;
  color: #303133;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.project-date {
  font-size: 12px;
  color: #909399;
  margin-top: 4px;
}

.project-actions {
  flex-shrink: 0;
}
</style>

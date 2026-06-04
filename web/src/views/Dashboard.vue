<template>
  <BaseLayout>
    <div class="dashboard">
      <!-- 内容区 -->
      <main class="dash-main">
        <div class="dash-toolbar">
          <h2>我的项目</h2>
          <el-button type="primary" :icon="Plus" @click="showCreateDialog = true">新建项目</el-button>
        </div>

        <!-- 项目列表 -->
        <div v-if="projects.length === 0" class="dash-empty">
          <el-empty description="还没有项目，点击上方按钮创建第一个 LaTeX 项目吧" />
        </div>

        <div v-else class="project-grid">
          <ProjectCard
            v-for="project in projects"
            :key="project.id"
            :project="project"
            @select="goToProject"
            @rename="openRename"
            @delete="handleDelete"
          />
        </div>
      </main>

      <!-- 新建项目对话框 -->
      <el-dialog v-model="showCreateDialog" title="新建项目" width="400px">
        <el-form ref="createFormRef" :model="createForm" :rules="createRules" @keyup.enter="handleCreate">
          <el-form-item prop="name">
            <el-input v-model="createForm.name" placeholder="项目名称" />
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="showCreateDialog = false">取消</el-button>
          <el-button type="primary" :loading="creating" @click="handleCreate">创建</el-button>
        </template>
      </el-dialog>

      <!-- 重命名对话框 -->
      <el-dialog v-model="showRenameDialog" title="重命名项目" width="400px">
        <el-form ref="renameFormRef" :model="renameForm" :rules="renameRules" @keyup.enter="handleRename">
          <el-form-item prop="name">
            <el-input v-model="renameForm.name" placeholder="新项目名称" />
          </el-form-item>
        </el-form>
        <template #footer>
          <el-button @click="showRenameDialog = false">取消</el-button>
          <el-button type="primary" :loading="renaming" @click="handleRename">确认</el-button>
        </template>
      </el-dialog>
    </div>
  </BaseLayout>
</template>

<script setup>
import { Plus } from "@element-plus/icons-vue";
import { ElMessageBox } from "element-plus";
import { ref, reactive, onMounted } from "vue";
import { useRouter } from "vue-router";
import { projectAPI } from "../api";
import ProjectCard from "../components/ProjectCard.vue";
import BaseLayout from "../layouts/BaseLayout.vue";

const router = useRouter();
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

function goToProject(project) {
  router.push(`/project/${project.id}`);
}

async function handleCreate() {
  const valid = await createFormRef.value.validate().catch(() => false);
  if (!valid) return;
  creating.value = true;
  try {
    const { data } = await projectAPI.create({ name: createForm.name });
    showCreateDialog.value = false;
    createForm.name = "";
    // 创建成功后直接跳入编辑器
    router.push(`/project/${data.id}`);
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
    await ElMessageBox.confirm(`确定要删除项目「${project.name}」吗？此操作不可恢复。`, "确认删除", {
      type: "warning",
      confirmButtonText: "删除",
      cancelButtonText: "取消",
    });
    await projectAPI.delete(project.id);
    await fetchProjects();
  } catch {
    /* cancelled */
  }
}
</script>

<style scoped>
.dashboard {
  height: 100%;
  background: var(--color-bg);
}

.dash-main {
  height: 100%;
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
  color: var(--color-text);
}

.dash-empty {
  margin-top: 80px;
}

.project-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
  gap: 16px;
}
</style>

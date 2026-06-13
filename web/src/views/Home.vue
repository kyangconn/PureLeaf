<template>
  <div class="home">
    <main class="home-main">
      <div class="home-toolbar">
        <h2>项目</h2>
      </div>

      <Loading v-if="loading" fullscreen>正在加载项目列表...</Loading>

      <ProjectList v-else :projects="projects" @select="goToProject" @rename="openRename" @delete="handleDelete" />
    </main>

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
</template>

<script setup>
import { useRouter } from "vue-router";
import { ElMessageBox } from "element-plus";
import { ref, reactive, onMounted } from "vue";

import { projectAPI } from "../api";
import Loading from "../components/Loading.vue";
import ProjectList from "../components/ProjectList.vue";

const router = useRouter();
const projects = ref([]);
const loading = ref(true);

const showRenameDialog = ref(false);
const renaming = ref(false);
const renameFormRef = ref(null);
const renameForm = reactive({ name: "" });
const renameTarget = ref(null);
const renameRules = { name: [{ message: "请输入新名称", required: true, trigger: "blur" }] };

onMounted(fetchProjects);

async function fetchProjects() {
  loading.value = true;
  try {
    const { data } = await projectAPI.list();
    projects.value = data || [];
  } catch {
    /* handled */
  } finally {
    loading.value = false;
  }
}

function goToProject(project) {
  router.push(`/project/${project.id}`);
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
      cancelButtonText: "取消",
      confirmButtonText: "删除",
      type: "warning",
    });
    await projectAPI.delete(project.id);
    await fetchProjects();
  } catch {
    /* cancelled */
  }
}
</script>

<style lang="scss" scoped>
@use "@/styles/variables" as *;
@use "@/styles/mixins" as *;

.home {
  height: 100%;
  background: $color-bg;
}

.home-main {
  position: relative;
  height: 100%;
  overflow-y: auto;
  padding: 24px;
  max-width: 800px;
  width: 100%;
  margin: 0 auto;
}

.home-toolbar {
  @include flex-between;
  margin-bottom: 20px;
  h2 {
    font-size: 20px;
    font-weight: 600;
    color: $color-text;
  }
}
</style>

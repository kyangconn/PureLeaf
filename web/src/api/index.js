// API 模块 — 封装所有后端接口调用
import axios from "axios";
import { ElMessage } from "element-plus";

const http = axios.create({
  baseURL: "/api",
  timeout: 30000,
});

// 请求拦截器 — 自动附加 JWT Token
http.interceptors.request.use((config) => {
  const token = localStorage.getItem("token");
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

// 响应拦截器 — 统一解包后端 Response{code,message,data} 信封
http.interceptors.response.use(
  (res) => {
    // 后端统一响应格式: {code, message, data} → 提取 data 字段
    const body = res.data;
    if (body && typeof body === "object" && "code" in body) {
      if ("data" in body && body.data !== undefined) {
        res.data = body.data;
      }
    }
    return res;
  },
  (err) => {
    const msg = err.response?.data?.error || err.message || "请求失败";
    ElMessage.error(msg);
    if (err.response?.status === 401) {
      localStorage.removeItem("token");
      localStorage.removeItem("user");
      window.location.href = "/login";
    }
    return Promise.reject(err);
  },
);

// ---- 认证 ----
export const authAPI = {
  status: () => http.get("/auth/status"),
  register: (data) => http.post("/auth/register", data),
  login: (data) => http.post("/auth/login", data),
  me: () => http.get("/auth/me"),
};

// ---- 项目 ----
export const projectAPI = {
  list: () => http.get("/projects"),
  get: (id) => http.get(`/projects/${id}`),
  create: (data) => http.post("/projects", data),
  update: (id, data) => http.put(`/projects/${id}`, data),
  delete: (id) => http.delete(`/projects/${id}`),
};

// ---- 文件 ----
export const fileAPI = {
  getTree: (projectId) => http.get(`/projects/${projectId}/files`),
  getContent: (projectId, fileId) => http.get(`/projects/${projectId}/files/${fileId}`),
  create: (projectId, data) => http.post(`/projects/${projectId}/files`, data),
  updateContent: (projectId, fileId, data) => http.put(`/projects/${projectId}/files/${fileId}`, data),
  rename: (projectId, fileId, data) => http.patch(`/projects/${projectId}/files/${fileId}/rename`, data),
  delete: (projectId, fileId) => http.delete(`/projects/${projectId}/files/${fileId}`),
  compile: (projectId) => http.post(`/projects/${projectId}/compile`, {}, { responseType: "blob" }),
};

export default http;

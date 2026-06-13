import {
  CompileProject,
  CreateFile,
  CreateProject,
  DeleteFile,
  DeleteProject,
  GetFileContent,
  GetFileTree,
  GetProject,
  ListProjects,
  RenameFile,
  UpdateFileContent,
  UpdateProject,
} from "@wailsjs/transport/App";

export const projectAPI = {
  create: (data: { name: string }) => CreateProject(data.name),
  delete: (id: number) => DeleteProject(id),
  get: (id: number) => GetProject(id),
  list: () => ListProjects(),
  update: (id: number, data: { name: string }) => UpdateProject(id, data.name),
};

export const fileAPI = {
  compile: async (projectId: number) => {
    const result = await CompileProject(projectId);
    const blob = new Blob([new Uint8Array(result.pdf)], { type: "application/pdf" });
    return { data: blob, log: result.log };
  },
  create: (projectId: number, data: { is_dir?: boolean; name: string; parent_id?: number }) =>
    CreateFile(projectId, data.name, data.parent_id ?? null, data.is_dir ?? false),
  delete: (projectId: number, fileId: number) => DeleteFile(projectId, fileId),
  getContent: (projectId: number, fileId: number) => GetFileContent(projectId, fileId),
  getTree: (projectId: number) => GetFileTree(projectId),
  rename: (projectId: number, fileId: number, data: { name: string }) => RenameFile(projectId, fileId, data.name),
  updateContent: (projectId: number, fileId: number, data: { content: string }) =>
    UpdateFileContent(projectId, fileId, data.content),
};

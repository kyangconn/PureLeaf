import {
  EnvironmentService,
  FileService,
  ProjectService,
} from "@bindings/github.com/kyangconn/goleaf/internal/bindings";

function decodeByteSlice(value: string) {
  const binary = atob(value);
  const bytes = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index += 1) {
    bytes[index] = binary.charCodeAt(index);
  }
  return bytes;
}

export const projectAPI = {
  create: (data: { name: string }) => ProjectService.CreateProject(data.name),
  delete: (id: number) => ProjectService.DeleteProject(id),
  get: (id: number) => ProjectService.GetProject(id),
  list: () => ProjectService.ListProjects(),
  update: (id: number, data: { name: string }) => ProjectService.UpdateProject(id, data.name),
};

export const fileAPI = {
  compile: async (projectId: number) => {
    const result = await FileService.CompileProject(projectId);
    if (!result) throw new Error("编译失败");
    const blob = new Blob([decodeByteSlice(result.pdf)], { type: "application/pdf" });
    return { data: blob, log: result.log };
  },
  create: (projectId: number, data: { is_dir?: boolean; name: string; parent_id?: number }) =>
    FileService.CreateFile(projectId, data.name, data.parent_id ?? null, data.is_dir ?? false),
  delete: (projectId: number, fileId: number) => FileService.DeleteFile(projectId, fileId),
  getContent: (projectId: number, fileId: number) => FileService.GetFileContent(projectId, fileId),
  getTree: (projectId: number) => FileService.GetFileTree(projectId),
  rename: (projectId: number, fileId: number, data: { name: string }) =>
    FileService.RenameFile(projectId, fileId, data.name),
  updateContent: (projectId: number, fileId: number, data: { content: string }) =>
    FileService.UpdateFileContent(projectId, fileId, data.content),
};

export const latexAPI = {
  checkEnvironment: () => EnvironmentService.CheckLatexEnvironment(),
  downloadTexLiveInstaller: (variant: "base" | "small" | "medium" | "full") =>
    EnvironmentService.DownloadTexLiveInstaller(variant),
  reloadEnvironment: () => EnvironmentService.ReloadLatexEnvironment(),
  startTexLiveInstaller: (variant: "base" | "small" | "medium" | "full") =>
    EnvironmentService.StartTexLiveInstaller(variant),
};

import { spawn, spawnSync } from "node:child_process";
import fs from "node:fs";
import net from "node:net";
import path from "node:path";
import process from "node:process";
import { fileURLToPath } from "node:url";

const scriptDir = path.dirname(fileURLToPath(import.meta.url));
const rootDir = path.resolve(scriptDir, "..");
const webDir = path.join(rootDir, "web");
const binDir = path.join(rootDir, "bin");
const devBinaryName = process.platform === "win32" ? "goleaf-dev.exe" : "goleaf-dev";
const devBinaryPath = path.join(binDir, devBinaryName);
const vitePort = Number(process.env.WAILS_VITE_PORT || "9245");

function runLongLived(command, args, options = {}) {
  const child = spawn(command, args, {
    cwd: options.cwd || rootDir,
    env: process.env,
    shell: options.shell || false,
    stdio: "inherit",
  });

  child.on("error", (error) => {
    console.error(error.message);
    process.exit(1);
  });

  child.on("exit", (code, signal) => {
    if (signal) {
      process.exit(1);
    }
    process.exit(code ?? 0);
  });
}

function runBlocking(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: options.cwd || rootDir,
    env: process.env,
    stdio: "inherit",
  });

  if (result.error) {
    console.error(result.error.message);
    process.exit(1);
  }

  process.exit(result.status ?? 0);
}

function frontend() {
  runLongLived(
    "pnpm",
    ["dev", "--", "--host", "127.0.0.1", "--port", String(vitePort), "--strictPort"],
    { cwd: webDir, shell: process.platform === "win32" },
  );
}

function killDevApp() {
  if (process.platform === "win32") {
    spawnSync("taskkill", ["/T", "/F", "/IM", devBinaryName], { stdio: "ignore" });
  }
}

function build() {
  fs.mkdirSync(binDir, { recursive: true });
  runBlocking("go", ["build", "-buildvcs=false", "-gcflags=all=-l", "-o", devBinaryPath, "."]);
}

function canConnect() {
  return new Promise((resolve) => {
    const socket = net.createConnection({ host: "127.0.0.1", port: vitePort });
    let settled = false;
    const done = (ready) => {
      if (settled) {
        return;
      }
      settled = true;
      socket.destroy();
      resolve(ready);
    };

    socket.setTimeout(250);
    socket.once("connect", () => done(true));
    socket.once("error", () => done(false));
    socket.once("timeout", () => done(false));
  });
}

async function waitForFrontend() {
  const deadline = Date.now() + 45_000;

  while (Date.now() < deadline) {
    if (await canConnect()) {
      return;
    }
    await new Promise((resolve) => setTimeout(resolve, 250));
  }

  console.error(`Timed out waiting for Vite on 127.0.0.1:${vitePort}`);
  process.exit(1);
}

function runApp() {
  runLongLived(devBinaryPath, []);
}

const command = process.argv[2];

switch (command) {
  case "frontend":
    frontend();
    break;
  case "kill":
    killDevApp();
    break;
  case "build":
    build();
    break;
  case "wait":
    await waitForFrontend();
    break;
  case "run":
    runApp();
    break;
  default:
    console.error("Usage: node scripts/wails-dev.mjs <frontend|kill|build|wait|run>");
    process.exit(1);
}

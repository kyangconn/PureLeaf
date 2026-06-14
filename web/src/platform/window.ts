import { Application, Window } from "@wailsio/runtime";

async function runWindowCommand(command: () => Promise<void>) {
  try {
    await command();
  } catch {
    // The command is unavailable when the frontend is previewed outside Wails.
  }
}

export function closeApp() {
  return runWindowCommand(() => Application.Quit());
}

export function minimizeWindow() {
  return runWindowCommand(() => Window.Minimise());
}

export function toggleMaximizeWindow() {
  return runWindowCommand(() => Window.ToggleMaximise());
}

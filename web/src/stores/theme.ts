import { defineStore } from "pinia";
import { computed, ref, watch } from "vue";

import { WindowSetDarkTheme, WindowSetLightTheme } from "../../wailsjs/runtime/runtime";

export type ThemeMode = "dark" | "light";

const APP_THEME_KEY = "goleaf:app-theme";
const EDITOR_THEME_KEY = "goleaf:editor-theme";

function readTheme(key: string, fallback: ThemeMode): ThemeMode {
  const value = localStorage.getItem(key);
  return value === "dark" || value === "light" ? value : fallback;
}

function syncWindowTheme(theme: ThemeMode) {
  const wailsWindow = window as Window & { runtime?: unknown };
  if (!wailsWindow.runtime) return;
  if (theme === "dark") WindowSetDarkTheme();
  else WindowSetLightTheme();
}

export const useThemeStore = defineStore("theme", () => {
  const appTheme = ref<ThemeMode>(readTheme(APP_THEME_KEY, "light"));
  const editorTheme = ref<ThemeMode>(readTheme(EDITOR_THEME_KEY, "light"));

  const appClasses = computed(() => ["app-shell", `app-theme-${appTheme.value}`, `editor-theme-${editorTheme.value}`]);
  const isAppDark = computed(() => appTheme.value === "dark");

  watch(
    appTheme,
    (value) => {
      localStorage.setItem(APP_THEME_KEY, value);
      syncWindowTheme(value);
    },
    { immediate: true },
  );

  watch(editorTheme, (value) => {
    localStorage.setItem(EDITOR_THEME_KEY, value);
  });

  function setAppTheme(theme: ThemeMode) {
    appTheme.value = theme;
  }

  function setEditorTheme(theme: ThemeMode) {
    editorTheme.value = theme;
  }

  return {
    appClasses,
    appTheme,
    editorTheme,
    isAppDark,
    setAppTheme,
    setEditorTheme,
  };
});

import css from "@eslint/css";
import globals from "globals";
import tseslint from "typescript-eslint";
import pluginVue from "eslint-plugin-vue";
import { defineConfig } from "eslint/config";
import perfectionist from "eslint-plugin-perfectionist";
import pluginPrettier from "eslint-plugin-prettier/recommended";

export default defineConfig([
  // ── Global ignores ──────────────────────────────────────────────
  {
    ignores: ["**/node_modules/**", "**/dist/**", "build/", "**/public/**", ".cache/", "bindings/", "wailsjs/"],
  },

  // ── JS + TS base ─────────────────────────────────────────────────
  ...tseslint.configs.recommended,

  // ── Vue (must come after tseslint so vue-eslint-parser takes priority) ──
  ...pluginVue.configs["flat/recommended"],
  {
    files: ["**/*.vue"],
    languageOptions: {
      parserOptions: {
        parser: tseslint.parser,
      },
    },
  },

  // ── Prettier (must be last to override formatting rules) ─────────
  pluginPrettier,

  // ── CSS ──────────────────────────────────────────────────────────
  {
    files: ["**/*.css"],
    language: "css/css",
    plugins: { css },
    rules: {
      "css/no-duplicate-imports": "error",
      "css/no-empty-blocks": "error",
    },
  },

  // ── Global settings ──────────────────────────────────────────────
  {
    languageOptions: {
      globals: { ...globals.browser, ...globals.node },
    },
    rules: {
      "@typescript-eslint/explicit-module-boundary-types": "off",
      "@typescript-eslint/no-explicit-any": "warn",
      "@typescript-eslint/no-unused-vars": ["warn", { argsIgnorePattern: "^_", varsIgnorePattern: "^_" }],
      "no-console": ["warn", { allow: ["warn", "error"] }],
    },
  },

  // ── Auto-sort imports ───────────────────────────────────────────
  {
    plugins: { perfectionist },
    rules: {
      "perfectionist/sort-imports": "error",
      "perfectionist/sort-interfaces": ["error"],
      "perfectionist/sort-objects": ["error", { type: "alphabetical" }],
    },
    settings: {
      perfectionist: {
        partitionByComment: true,
        type: "line-length",
      },
    },
  },
]);

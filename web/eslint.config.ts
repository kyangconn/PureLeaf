import js from "@eslint/js";
import css from "@eslint/css";
import globals from "globals";
import tseslint from "typescript-eslint";
import pluginVue from "eslint-plugin-vue";
import perfectionist from "eslint-plugin-perfectionist";
import pluginPrettier from "eslint-plugin-prettier/recommended";

export default [
  // ── Global ignores ──────────────────────────────────────────────
  {
    ignores: ["**/node_modules/**", "**/dist/**", "build/", "**/public/**", ".cache/", "wailsjs/"],
  },

  js.configs.recommended,
  pluginPrettier,
  ...pluginVue.configs["flat/recommended"],
  ...tseslint.configs.recommended,

  {
    files: ["**/*.css"],
    language: "css/css",
    plugins: {
      css,
    },
    rules: {
      "css/no-duplicate-imports": "error",
      "css/no-empty-blocks": "error",
    },
  },
  {
    languageOptions: {
      globals: { ...globals.browser, ...globals.node },
    },
    rules: {
      "@typescript-eslint/no-explicit-any": "warn",
      "@typescript-eslint/no-unused-vars": ["warn", { argsIgnorePattern: "^_", varsIgnorePattern: "^_" }],
      "no-console": ["warn", { allow: ["warn", "error"] }],
    },
  },

  // ── Auto-sort imports ──────────────────────────────────────────
  {
    plugins: {
      perfectionist,
    },
    rules: {
      "perfectionist/sort-imports": "error",
      "perfectionist/sort-interfaces": ["error"],
      "perfectionist/sort-objects": [
        "error",
        {
          type: "alphabetical",
        },
      ],
    },
    settings: {
      perfectionist: {
        partitionByComment: true,
        type: "line-length",
      },
    },
  },
];

import { resolve } from "path";
import { defineConfig } from "vite";
import preact from "@preact/preset-vite";
import tailwindcss from "@tailwindcss/vite";

export default defineConfig({
  plugins: [preact(), tailwindcss()],
  build: {
    outDir: resolve(__dirname, "../data"),
    emptyOutDir: true,
    assetsDir: "",
  },
  server: {
    host: "127.0.0.1",
  },
});

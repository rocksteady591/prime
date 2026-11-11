import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import path from 'path'
import fs from 'fs'

export default defineConfig({
  plugins: [vue()],
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
  server: {
    host: '127.0.0.1',
    port: 5173,
    https: {
      key: fs.readFileSync(path.resolve(__dirname, '127.0.0.1+1-key.pem')),
      cert: fs.readFileSync(path.resolve(__dirname, '127.0.0.1+1.pem')),
    },
    hmr: {
      protocol: 'wss',
      host: '127.0.0.1',
    },
    proxy: {
      '/api': {
        target: 'https://127.0.0.1:8081',
        changeOrigin: true,
        secure: false,
      },
      '/ws': {
        target: 'wss://127.0.0.1:9000',
        ws: true,
        secure: false,
        changeOrigin: true,
      },
    },
  },
})
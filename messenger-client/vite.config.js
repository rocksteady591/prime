import { defineConfig } from 'vite';
import vue from '@vitejs/plugin-vue';
import path from 'path';
// https://vite.dev/config/
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
        proxy: {
            '/ws': {
                target: 'ws://127.0.0.1:9000',
                ws: true,
                changeOrigin: true,
                rewrite: (path) => path,
                configure: (proxy, options) => {
                    proxy.on('error', (err, req, res) => {
                        console.log('proxy error', err);
                    });
                    proxy.on('proxyReq', (proxyReq, req, res) => {
                        console.log('proxy request', req.url);
                    });
                },
            },
            '/api': {
                target: 'https://localhost:8081', // адрес HTTP-сервера
                changeOrigin: true,
                secure: false,
            },
        },
    },
});

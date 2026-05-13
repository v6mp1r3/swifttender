import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': {
        target: 'http://localhost:8000',
        changeOrigin: true,
      }
    }
  },
  build: {
    /* Production build goes into backend/data/www
       so the C server can serve it as static files */
    outDir: '../backend/data/www',
    emptyOutDir: true,
  }
})

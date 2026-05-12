import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

/*
 * vite.config.js
 *
 * In development:  Vite serves the React app on :5173 and proxies
 *                  all /api/* requests to the C backend on :8000.
 *
 * In production:   `npm run build` outputs to dist/, which the C
 *                  backend serves directly as static files.
 */
export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': {
        target:       'http://localhost:8000',
        changeOrigin: true,
      }
    }
  },
  build: {
    outDir:    '../backend/data/www',  /* served by mongoose as static root */
    emptyOutDir: true,
  }
})

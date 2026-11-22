import { createReadStream, existsSync, statSync } from 'node:fs';
import { extname, resolve, sep } from 'node:path';
import { defineConfig, type Plugin } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';

const MIME: Record<string, string> = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript',
  '.css': 'text/css',
  '.json': 'application/json',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.woff2': 'font/woff2',
  '.woff': 'font/woff',
};

/** /prod/ on the dev server serves the last production build (dist/), so the Pi can try the
 *  optimised version over the same port as the dev server (tools/kiosk-url.sh prod). */
function serveBuild(): Plugin {
  const dist = resolve(import.meta.dirname, 'dist');
  return {
    name: 'serve-build',
    configureServer(server) {
      server.middlewares.use('/prod', (req, res, next) => {
        const path = decodeURIComponent((req.url ?? '/').split('?')[0]);
        const file = resolve(dist, '.' + (path.endsWith('/') ? path + 'index.html' : path));
        if (!file.startsWith(dist + sep) || !existsSync(file) || statSync(file).isDirectory()) return next();
        res.setHeader('Content-Type', MIME[extname(file)] ?? 'application/octet-stream');
        res.setHeader('Cache-Control', 'no-store');
        createReadStream(file).pipe(res);
      });
    },
  };
}

export default defineConfig({
  plugins: [svelte(), serveBuild()],
  // Relative asset paths: the same build works from dashboard-server and from file://.
  base: './',
  build: {
    // Only WPE WebKit 2.54 on the Pi has to run this.
    target: 'es2022',
    assetsInlineLimit: 0,
  },
  server: {
    // Reachable from the Pi, so the kiosk can load the dev server (tools/kiosk-url.sh dev).
    host: true,
    port: 5173,
    strictPort: true,
    proxy: { '/api': 'http://127.0.0.1:8080' },
  },
});

import { mount } from 'svelte';
import '@fontsource-variable/inter';
import './app.css';
import App from './App.svelte';
import { connect } from './lib/state.svelte';

// On the Pi the window is exactly 1280x720. In a desktop browser, scale the screen to fit,
// so the layout can be judged in any window size.
function fit(): void {
  const scale = Math.min(window.innerWidth / 1280, window.innerHeight / 720);
  document.documentElement.style.setProperty('--fit', String(Math.abs(scale - 1) < 0.01 ? 1 : scale));
}
fit();
window.addEventListener('resize', fit);

mount(App, { target: document.getElementById('app')! });
void connect();

// ?bench plays the transitions by itself and logs their frame rate (src/lib/bench.ts).
const params = new URLSearchParams(location.search);
// ?flat: no box shadows anywhere, to measure what painting them costs.
if (params.has('flat')) document.documentElement.classList.add('flat');
if (params.has('bench')) {
  const flags = ['still', 'flat'].filter((f) => params.has(f));
  const label = [import.meta.env.DEV ? 'dev' : 'prod', ...flags].join('+');
  void import('./lib/bench').then((m) => m.runBench(label));
}

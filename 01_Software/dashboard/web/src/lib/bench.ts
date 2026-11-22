// ?bench: plays the dashboard's transitions by itself and logs how smooth each one was to the
// console, which on the Pi ends up in the journal:
//   journalctl -u dashboard-kiosk -o cat | grep BENCH
// Frames are counted with requestAnimationFrame. WPE paces it by the compositor, so slow
// compositing shows up as well as a busy main thread. Each step is triggered inside the
// measured window, so the work of starting it (new DOM, style) counts too.
import { closeSaver, go, nav, openSaver, SUMMARY, toggleOverview } from './nav.svelte';

const WINDOW_MS = 1000; // counted after each trigger; the transitions take up to 900 ms
const GAP_MS = 700; // calm time between steps
const ROUNDS = Number(new URLSearchParams(location.search).get('rounds')) || 3;
const SLOW_MS = 25; // a frame this long missed the 60 Hz display by more than half a frame

interface Step {
  kind: string; // the group in the summary
  name: string;
  run(): void;
}

const STEPS: Step[] = [
  { kind: 'idle', name: 'idle', run: () => {} },
  { kind: 'swipe', name: 'swipe→weather', run: () => go(SUMMARY + 1) },
  { kind: 'swipe', name: 'swipe→music', run: () => go(SUMMARY + 2) },
  { kind: 'swipe', name: 'swipe→weather', run: () => go(SUMMARY + 1) },
  { kind: 'swipe', name: 'swipe→summary', run: () => go(SUMMARY) },
  { kind: 'swipe', name: 'swipe→calendar', run: () => go(SUMMARY - 1) },
  { kind: 'swipe', name: 'swipe→printer', run: () => go(SUMMARY - 2) },
  { kind: 'swipe', name: 'swipe→home', run: () => go(SUMMARY - 3) },
  { kind: 'swipe', name: 'swipe→printer', run: () => go(SUMMARY - 2) },
  { kind: 'swipe', name: 'swipe→calendar', run: () => go(SUMMARY - 1) },
  { kind: 'swipe', name: 'swipe→summary', run: () => go(SUMMARY) },
  { kind: 'overview', name: 'overview open', run: () => toggleOverview(true) },
  { kind: 'overview', name: 'overview→photos', run: () => go('photos') },
  { kind: 'overview', name: 'overview open', run: () => toggleOverview(true) },
  { kind: 'overview', name: 'overview close', run: () => toggleOverview(false) },
  { kind: 'overview', name: 'overview open', run: () => toggleOverview(true) },
  { kind: 'overview', name: 'overview→summary', run: () => go(SUMMARY) },
  { kind: 'controls', name: 'controls open', run: () => (nav.controls = true) },
  { kind: 'controls', name: 'controls close', run: () => (nav.controls = false) },
  { kind: 'saver', name: 'saver open', run: openSaver },
  { kind: 'saver', name: 'saver close', run: closeSaver },
];

const sleep = (ms: number) => new Promise((r) => setTimeout(r, ms));

/** Frame times for WINDOW_MS; `start` runs in the first frame. */
function frames(start: () => void): Promise<number[]> {
  return new Promise((resolve) => {
    const times: number[] = [];
    const frame = (t: number) => {
      if (!times.length) start();
      times.push(t);
      if (t - times[0] < WINDOW_MS) requestAnimationFrame(frame);
      else resolve(times);
    };
    requestAnimationFrame(frame);
  });
}

interface Result {
  gaps: number[]; // ms between frames
}

function describe(gaps: number[]): string {
  const total = gaps.reduce((a, b) => a + b, 0);
  const fps = Math.round((gaps.length / total) * 1000);
  const worst = Math.round(Math.max(...gaps));
  const slow = gaps.filter((g) => g > SLOW_MS).length;
  return `fps=${fps} worst=${worst}ms slow=${slow}/${gaps.length}`;
}

/** Where the slow frames fell: frame number and length, e.g. "1:141 2:38". */
function where(gaps: number[]): string {
  return gaps
    .map((g, i) => [i + 1, Math.round(g)])
    .filter(([, g]) => g > SLOW_MS)
    .map(([i, g]) => `${i}:${g}`)
    .join(' ');
}

export async function runBench(label: string): Promise<void> {
  await sleep(5000); // the test photos and fonts are ready by then
  console.log(`BENCH ${label} start, ${ROUNDS} rounds`);
  const byKind = new Map<string, Result>();
  for (let round = 1; round <= ROUNDS; round++) {
    for (const step of STEPS) {
      const times = await frames(step.run);
      const gaps = times.slice(1).map((t, i) => t - times[i]);
      console.log(`BENCH ${label} r${round} ${step.name.padEnd(17)} ${describe(gaps)}  [${where(gaps)}]`);
      const r = byKind.get(step.kind) ?? { gaps: [] };
      r.gaps.push(...gaps);
      byKind.set(step.kind, r);
      await sleep(step.kind === 'saver' ? GAP_MS + 1000 : GAP_MS);
    }
  }
  for (const [kind, r] of byKind) console.log(`BENCH ${label} TOTAL ${kind.padEnd(9)} ${describe(r.gaps)}`);
  console.log(`BENCH ${label} done`);
}

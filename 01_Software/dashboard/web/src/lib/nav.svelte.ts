// Pages, overview and screensaver state.
// URL parameters for testing: ?page=weather ?overview ?controls ?saver (screensaver at once)
// ?saverAfter=20 (seconds of idle time before the screensaver, over the setting saverAfterS).
import type { Component } from 'svelte';
import CalendarDays from '@lucide/svelte/icons/calendar-days';
import CloudSun from '@lucide/svelte/icons/cloud-sun';
import House from '@lucide/svelte/icons/house';
import Images from '@lucide/svelte/icons/images';
import LayoutGrid from '@lucide/svelte/icons/layout-grid';
import Music from '@lucide/svelte/icons/music';
import Printer from '@lucide/svelte/icons/printer';
import { settings } from './state.svelte';

export type PageId = 'home' | 'printer' | 'calendar' | 'summary' | 'weather' | 'music' | 'photos';

export interface PageInfo {
  id: PageId;
  title: string;
  icon: Component;
}

/** Left to right; the summary sits in the middle. */
export const PAGES: PageInfo[] = [
  { id: 'home', title: 'Dom', icon: House },
  { id: 'printer', title: 'Drukarka', icon: Printer },
  { id: 'calendar', title: 'Kalendarz', icon: CalendarDays },
  { id: 'summary', title: 'Podsumowanie', icon: LayoutGrid },
  { id: 'weather', title: 'Pogoda', icon: CloudSun },
  { id: 'music', title: 'Muzyka', icon: Music },
  { id: 'photos', title: 'Zdjęcia', icon: Images },
];

export const SUMMARY = PAGES.findIndex((p) => p.id === 'summary');

const params = new URLSearchParams(location.search);
const startPage = PAGES.findIndex((p) => p.id === params.get('page'));

export const nav = $state({
  index: startPage >= 0 ? startPage : SUMMARY,
  overview: params.has('overview'),
  saver: params.has('saver'),
  controls: params.has('controls'), // the control bar pulled down from the top
  settings: params.has('settings'), // the settings sheet, opened from the control bar
  pull: null as number | null, // finger offset while pulling the control bar
  waking: false, // the screensaver is about to fade out; the pages are being painted under it
});

/** Runs fn after the next frame has been drawn, so whatever that frame had to paint is done
 *  before fn starts an animation. Painting a whole page takes the Pi tens of ms; during an
 *  animation that is a visible jerk, before it only a short delay. */
function afterPaint(fn: () => void): void {
  requestAnimationFrame(() => requestAnimationFrame(fn));
}

export function go(target: number | PageId): void {
  const i = typeof target === 'number' ? target : PAGES.findIndex((p) => p.id === target);
  if (i < 0) return;
  const index = Math.max(0, Math.min(PAGES.length - 1, i));
  if (nav.overview && index !== nav.index) {
    // The track jumps while the overview still hides it; the overview closes once the page
    // is painted, so its zoom runs smoothly.
    nav.index = index;
    afterPaint(() => (nav.overview = false));
    return;
  }
  nav.index = index;
  nav.overview = false;
}

export function toggleOverview(open = !nav.overview): void {
  nav.overview = open;
}

// Screensaver after a period without touch. It returns to the summary underneath, so a tap
// that closes it lands on the overview of everything. The time is set in the settings
// (saverAfterS, 0: never).
const saverAfterParam = Number(params.get('saverAfter')) || 0;
function saverAfterMs(): number {
  return (saverAfterParam || settings().saverAfterS) * 1000;
}
let lastTouch = Date.now();

export function touched(): void {
  lastTouch = Date.now();
}

export function closeSaver(): void {
  touched();
  if (!nav.saver || nav.waking) return;
  nav.waking = true;
  afterPaint(() => {
    nav.saver = false;
    nav.waking = false;
  });
}

export function openSaver(): void {
  nav.controls = false;
  nav.overview = false;
  nav.saver = true;
}

window.addEventListener('pointerdown', touched, { capture: true, passive: true });
setInterval(() => {
  const after = saverAfterMs();
  if (after > 0 && !nav.saver && Date.now() - lastTouch > after) {
    nav.saver = true;
    nav.overview = false;
    nav.controls = false;
    nav.index = SUMMARY;
  }
}, 5000);

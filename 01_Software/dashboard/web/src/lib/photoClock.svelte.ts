// One photo at a time, everywhere: the summary tile, the overview tile, the photos page and the
// screensaver all show photoClock.index of dash.photos.items. It moves on every
// settings.photoIntervalS seconds (after decoding the next photo, so the crossfades start
// with a ready picture); a tap on the photos page moves it at once and restarts the wait.
// A new batch from the server starts again at its first photo.
import { untrack } from 'svelte';
import { dash, settings } from './state.svelte';
import type { Photo } from './types';

export const photoClock = $state({ index: 0 });

function items(): Photo[] {
  return dash.photos?.items ?? [];
}

export function currentPhoto(): Photo | null {
  const list = items();
  return list.length ? list[photoClock.index % list.length] : null;
}

/** Fetches and decodes a photo off the main thread; later <img> uses reuse the result. */
export function preload(url: string): Promise<void> {
  const img = new Image();
  img.src = url;
  return img.decode().catch(() => {}); // a broken photo still gets its turn
}

let timer: ReturnType<typeof setTimeout> | undefined;
let generation = 0; // a tap during the decode of the next photo wins over the timer

function schedule(): void {
  clearTimeout(timer);
  const list = items();
  if (list.length < 2) return;
  const mine = ++generation;
  timer = setTimeout(async () => {
    const next = (photoClock.index + 1) % list.length;
    await preload(list[next].url);
    if (mine !== generation) return;
    photoClock.index = next;
    schedule();
  }, settings().photoIntervalS * 1000);
}

/** Moves by `by` photos now (a tap) and waits a full interval for the next. */
export function stepPhoto(by: number): void {
  const n = items().length;
  if (!n) return;
  photoClock.index = (((photoClock.index + by) % n) + n) % n;
  schedule();
}

// A new batch starts at its first photo; another interval only restarts the wait.
let batch = '';
$effect.root(() => {
  $effect(() => {
    const first = items()[0]?.id ?? '';
    void settings().photoIntervalS;
    untrack(() => {
      if (first !== batch) {
        batch = first;
        photoClock.index = 0;
      }
      schedule();
    });
  });
});

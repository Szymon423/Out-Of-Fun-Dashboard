// Generated landscapes standing in for Immich photos: in test mode, and as placeholders while
// the server has no photos (before stage D3), so the screensaver never shows a bare clock.
// Drawn once on a canvas and kept as JPEG blobs, so showing them costs the same as real photos.
import type { Photo, Photos } from './types';

const PALETTES: [string, string, string, string][] = [
  ['#0b1d3a', '#3a5fa8', '#f6b37a', '#ffe1a8'], // dusk over mountains
  ['#4f9fe8', '#9fd2ff', '#fff1c9', '#ffd27a'], // bright day
  ['#14080e', '#5a1f3a', '#d9603b', '#ffb067'], // sunset
  ['#081226', '#1d2e5c', '#6d4aa8', '#e39ad0'], // violet night
  ['#2d6f8e', '#8cc7d8', '#f3e6c4', '#f5b98a'], // morning by the lake
  ['#1b2b22', '#4c7a5d', '#d8d29a', '#f0c27a'], // green valley
];

const PLACES = ['Zakopane', 'Bieszczady', 'Hel', 'Karkonosze', 'Mazury', 'Pieniny'];

function mix(a: string, b: string, t: number): string {
  const pa = parseInt(a.slice(1), 16);
  const pb = parseInt(b.slice(1), 16);
  const m = (s: number) => Math.round(((pa >> s) & 255) * (1 - t) + ((pb >> s) & 255) * t);
  return `rgb(${m(16)},${m(8)},${m(0)})`;
}

function seeded(seed: number): () => number {
  let x = seed * 9301 + 49297;
  return () => {
    x = (x * 9301 + 49297) % 233280;
    return x / 233280;
  };
}

/** A blurred copy: shrink in two steps, then grow back with smoothing. Done once per photo;
 *  shown stretched, it needs no CSS filter at run time. */
function blurred(source: HTMLCanvasElement): string {
  const step = (from: CanvasImageSource, w: number, h: number) => {
    const c = document.createElement('canvas');
    c.width = w;
    c.height = h;
    const g = c.getContext('2d')!;
    g.imageSmoothingEnabled = true;
    g.imageSmoothingQuality = 'high';
    g.drawImage(from, 0, 0, w, h);
    return c;
  };
  const tiny = step(step(source, 160, 90), 32, 18);
  return step(step(tiny, 96, 54), 320, 180).toDataURL('image/jpeg', 0.85);
}

async function landscape(index: number, w = 1280, h = 720): Promise<{ url: string; blurUrl: string }> {
  const rnd = seeded(index + 7);
  const p = PALETTES[index % PALETTES.length];
  const c = document.createElement('canvas');
  c.width = w;
  c.height = h;
  const g = c.getContext('2d')!;

  const sky = g.createLinearGradient(0, 0, 0, h);
  sky.addColorStop(0, p[0]);
  sky.addColorStop(0.55, p[1]);
  sky.addColorStop(0.85, p[2]);
  sky.addColorStop(1, p[3]);
  g.fillStyle = sky;
  g.fillRect(0, 0, w, h);

  if (index % 2 === 0) {
    g.fillStyle = '#fff';
    for (let i = 0; i < 220; i++) {
      g.globalAlpha = rnd() * 0.7;
      const s = rnd() * 1.6;
      g.fillRect(rnd() * w, rnd() * h * 0.5, s, s);
    }
    g.globalAlpha = 1;
  }

  const sx = w * (0.25 + rnd() * 0.5);
  const sy = h * (0.3 + rnd() * 0.18);
  const sun = g.createRadialGradient(sx, sy, 0, sx, sy, h * 0.5);
  sun.addColorStop(0, 'rgba(255,244,220,1)');
  sun.addColorStop(0.12, 'rgba(255,226,180,.85)');
  sun.addColorStop(1, 'rgba(255,200,150,0)');
  g.fillStyle = sun;
  g.fillRect(0, 0, w, h);

  for (let layer = 0; layer < 5; layer++) {
    const base = h * (0.5 + layer * 0.1);
    const amp = h * (0.17 - layer * 0.025);
    const f1 = 0.002 + rnd() * 0.003;
    const f2 = 0.009 + rnd() * 0.009;
    const ph = rnd() * 100;
    g.beginPath();
    g.moveTo(0, h);
    for (let x = 0; x <= w; x += 6) {
      const y = base - amp * (0.6 * Math.sin(x * f1 + ph) + 0.3 * Math.sin(x * f2 + ph * 1.7) + 0.1 * Math.sin(x * f2 * 3.1 + ph));
      g.lineTo(x, y);
    }
    g.lineTo(w, h);
    g.closePath();
    g.fillStyle = mix(p[1], '#05070c', 0.35 + layer * 0.14);
    g.fill();
  }

  const blob = await new Promise<Blob | null>((r) => c.toBlob(r, 'image/jpeg', 0.88));
  return { url: blob ? URL.createObjectURL(blob) : c.toDataURL('image/jpeg', 0.88), blurUrl: blurred(c) };
}

/** `placeholder`: no made-up Immich memory, an album name that says what these are. */
export async function mockPhotos(placeholder = false): Promise<Photos> {
  const items: Photo[] = [];
  for (let i = 0; i < PALETTES.length; i++) {
    const taken = new Date();
    taken.setFullYear(taken.getFullYear() - 1 - (i % 4));
    const { url, blurUrl } = await landscape(i);
    items.push({ id: `mock-${i}`, url, blurUrl, takenAt: taken.toISOString(), place: PLACES[i] });
  }
  if (placeholder) return { album: 'Pejzaże', items, memory: null };
  return { album: 'Ulubione', items, memory: { title: 'Rok temu', photo: items[0] } };
}

// Light and dark theme. "auto" follows sunrise and sunset from the forecast (07:00 and 19:00
// without one). A manual choice holds until the next sunrise or sunset, then auto takes over
// again, like the automatic appearance on iOS.
import { clock } from './clock.svelte';
import { dash } from './state.svelte';

export type ThemeMode = 'auto' | 'light' | 'dark';
export type Theme = 'light' | 'dark';

const KEY = 'dashboard.theme';

interface Stored {
  mode: ThemeMode;
  until: number; // epoch ms; a manual mode is ignored after this
}

function read(): Stored {
  const forced = new URLSearchParams(location.search).get('theme');
  if (forced === 'light' || forced === 'dark') return { mode: forced, until: Number.MAX_SAFE_INTEGER };
  try {
    const v = JSON.parse(localStorage.getItem(KEY) ?? '') as Stored;
    if (v && (v.mode === 'auto' || v.mode === 'light' || v.mode === 'dark')) return v;
  } catch {
    // nothing stored yet
  }
  return { mode: 'auto', until: 0 };
}

export const theme: Stored = $state(read());

function sunTimes(now: Date): [Date, Date] {
  const t = dash.weather?.today;
  if (t) return [new Date(t.sunrise), new Date(t.sunset)];
  const rise = new Date(now);
  rise.setHours(7, 0, 0, 0);
  const set = new Date(now);
  set.setHours(19, 0, 0, 0);
  return [rise, set];
}

export function daylight(now: Date): boolean {
  const [rise, set] = sunTimes(now);
  return now >= rise && now < set;
}

/** The next sunrise or sunset after now. */
function nextSwitch(now: Date): number {
  const [rise, set] = sunTimes(now);
  const candidates = [rise, set, new Date(rise.getTime() + 86_400_000), new Date(set.getTime() + 86_400_000)];
  return Math.min(...candidates.map((d) => d.getTime()).filter((t) => t > now.getTime()));
}

export function resolvedTheme(): Theme {
  const now = clock.now;
  if (theme.mode !== 'auto' && now.getTime() < theme.until) return theme.mode;
  return daylight(now) ? 'light' : 'dark';
}

/** The mode shown in the switch: a manual mode that already expired counts as auto. */
export function activeMode(): ThemeMode {
  return theme.mode !== 'auto' && clock.now.getTime() < theme.until ? theme.mode : 'auto';
}

export function setThemeMode(mode: ThemeMode): void {
  theme.mode = mode;
  theme.until = mode === 'auto' ? 0 : nextSwitch(new Date());
  try {
    localStorage.setItem(KEY, JSON.stringify({ mode: theme.mode, until: theme.until }));
  } catch {
    // no storage: the choice lasts until the page reloads
  }
}

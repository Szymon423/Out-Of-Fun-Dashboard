// Labels and colours for the Bambu Lab printer state.
import type { Printer } from './types';

export const PRINTER_STATE: Record<Printer['state'], { label: string; color: string }> = {
  offline: { label: 'Wyłączona', color: 'var(--faint)' },
  idle: { label: 'Gotowa', color: 'var(--muted)' },
  printing: { label: 'Drukuje', color: 'var(--good)' },
  paused: { label: 'Wstrzymana', color: 'var(--accent)' },
  finished: { label: 'Wydruk gotowy', color: 'var(--good)' },
  error: { label: 'Wymaga uwagi', color: 'var(--bad)' },
};

/** "47 min", "2 h 05 min" */
export function remaining(min: number): string {
  if (min < 60) return `${Math.round(min)} min`;
  const h = Math.floor(min / 60);
  return `${h} h ${String(Math.round(min % 60)).padStart(2, '0')} min`;
}

/** Clock time when the job ends, from the remaining minutes. */
export function finishesAt(min: number): string {
  const end = new Date(Date.now() + min * 60_000);
  return end.toLocaleTimeString('pl-PL', { hour: '2-digit', minute: '2-digit' });
}

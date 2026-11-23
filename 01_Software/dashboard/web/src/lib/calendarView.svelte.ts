// Calendar view state: day, week or month, the date in view, and which sources are shown.
// The source filter applies everywhere, also to the summary widget. Remembered in this browser.
import { dash } from './state.svelte';
import type { CalendarEvent } from './types';

export type CalendarMode = 'day' | 'week' | 'month';

const KEY = 'dashboard.calendar';

function read(): { mode: CalendarMode; hidden: string[] } {
  try {
    const v = JSON.parse(localStorage.getItem(KEY) ?? '');
    if (v && ['day', 'week', 'month'].includes(v.mode) && Array.isArray(v.hidden)) return v;
  } catch {
    // nothing stored yet
  }
  return { mode: 'week', hidden: [] };
}

const saved = read();
// ?calendar=day|week|month opens that view (for checking a view without tapping).
const forced = new URLSearchParams(location.search).get('calendar');
if (forced === 'day' || forced === 'week' || forced === 'month') saved.mode = forced;

export const calView = $state({
  mode: saved.mode,
  hidden: saved.hidden, // source ids switched off
  anchor: new Date(), // a day inside the period in view
  selected: null as CalendarEvent | null, // shown in the event sheet (EventSheet.svelte)
});

function save(): void {
  try {
    localStorage.setItem(KEY, JSON.stringify({ mode: calView.mode, hidden: calView.hidden }));
  } catch {
    // no storage: lasts until the page reloads
  }
}

export function setMode(mode: CalendarMode): void {
  calView.mode = mode;
  save();
}

export function toggleSource(id: string): void {
  calView.hidden = calView.hidden.includes(id) ? calView.hidden.filter((h) => h !== id) : [...calView.hidden, id];
  save();
}

export function showDay(day: Date): void {
  calView.anchor = new Date(day);
  setMode('day');
}

export function today(): void {
  calView.anchor = new Date();
}

/** Moves the view one day, week or month back (-1) or forward (+1). */
export function step(direction: -1 | 1): void {
  const d = new Date(calView.anchor);
  if (calView.mode === 'day') d.setDate(d.getDate() + direction);
  else if (calView.mode === 'week') d.setDate(d.getDate() + 7 * direction);
  else d.setMonth(d.getMonth() + direction, 1);
  calView.anchor = d;
}

/** Events of the sources that are switched on. */
export function visibleEvents(): CalendarEvent[] {
  return (dash.events ?? []).filter((e) => !calView.hidden.includes(e.calendarId));
}

export function startOfDay(d: Date): Date {
  const x = new Date(d);
  x.setHours(0, 0, 0, 0);
  return x;
}

/** Monday of the week that contains d. */
export function startOfWeek(d: Date): Date {
  const x = startOfDay(d);
  x.setDate(x.getDate() - ((x.getDay() + 6) % 7));
  return x;
}

export function addDays(d: Date, n: number): Date {
  const x = new Date(d);
  x.setDate(x.getDate() + n);
  return x;
}

export function sameDay(a: Date, b: Date): boolean {
  return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
}

/** Events that touch the given day, all-day ones first, then by start. */
export function eventsOn(day: Date, list: CalendarEvent[]): CalendarEvent[] {
  const from = startOfDay(day).getTime();
  const to = from + 86_400_000;
  return list
    .filter((e) => new Date(e.start).getTime() < to && new Date(e.end).getTime() > from)
    .sort((a, b) => Number(b.allDay) - Number(a.allDay) || a.start.localeCompare(b.start));
}

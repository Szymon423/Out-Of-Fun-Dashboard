// Test data for working on the look without the server (?mock, or no server reachable).
// Built around the current time, so the agenda and the hourly forecast always look live.
import type { CalendarEvent, CalendarSource, DashboardState, Printer, WeatherDay, WeatherHour } from './types';

function at(hoursFromNow: number, minute = 0): Date {
  const d = new Date();
  d.setHours(d.getHours() + hoursFromNow, minute, 0, 0);
  return d;
}

function iso(d: Date): string {
  return d.toISOString();
}

function hours(): WeatherHour[] {
  const codes = [3, 3, 61, 63, 61, 3, 2, 2, 1, 0, 0, 1, 1, 2, 2, 3, 3, 61, 61, 3, 2, 1, 1, 0];
  const temps = [5, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0, -1, -1, 0, 1, 3, 4, 6, 7, 7, 6, 5, 4];
  return codes.map((code, i) => {
    const t = at(i + 1);
    const h = t.getHours();
    return {
      time: iso(t),
      temp: temps[i],
      code,
      isDay: h >= 7 && h < 16,
      precipProb: code >= 61 ? 60 + i * 3 : i < 6 ? 15 : 5,
      precipMm: code === 63 ? 1.2 : code === 61 ? 0.4 : 0,
    };
  });
}

function days(): WeatherDay[] {
  const list: [number, number, number, number][] = [
    [61, 6, 2, 80],
    [3, 4, 0, 30],
    [71, 1, -3, 60],
    [2, 2, -4, 10],
    [0, 3, -5, 0],
    [1, 5, -1, 5],
    [63, 7, 3, 85],
  ];
  return list.map(([code, max, min, precipProb], i) => {
    const d = new Date();
    d.setDate(d.getDate() + i + 1);
    const date = `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, '0')}-${String(d.getDate()).padStart(2, '0')}`;
    return { date, code, max, min, precipProb };
  });
}

export const MOCK_CALENDARS: CalendarSource[] = [
  { id: 'private', name: 'Prywatny', color: '#ff9f0a' },
  { id: 'm4t', name: 'M4T', color: '#0a84ff' },
  { id: 'itens', name: 'Itens', color: '#bf5af2' },
];

/** A plausible six weeks around today: work meetings on weekdays, private things in the
 *  evenings and at weekends, and two events relative to now, so "trwa" and "za X min" show. */
function events(): CalendarEvent[] {
  const now = new Date();
  const cal = Object.fromEntries(MOCK_CALENDARS.map((c) => [c.id, c]));
  const list: CalendarEvent[] = [];
  let n = 0;
  const add = (calendarId: string, title: string, start: Date, minutes: number, extra: Partial<CalendarEvent> = {}) => {
    const c = cal[calendarId];
    list.push({
      id: `mock-${n++}`,
      calendarId,
      title,
      start: iso(start),
      end: iso(new Date(start.getTime() + minutes * 60_000)),
      allDay: false,
      calendar: c.name,
      color: c.color,
      ...extra,
    });
  };
  const at = (day: Date, h: number, m = 0) => {
    const d = new Date(day);
    d.setHours(h, m, 0, 0);
    return d;
  };

  const first = new Date(now);
  first.setHours(0, 0, 0, 0);
  first.setDate(first.getDate() - 21);
  for (let i = 0; i < 49; i++) {
    const day = new Date(first);
    day.setDate(first.getDate() + i);
    const wd = day.getDay(); // 0 = Sunday
    const week = Math.floor(i / 7);
    if (wd >= 1 && wd <= 5) {
      add('m4t', 'Daily zespołu', at(day, 9, 30), 15);
      if (wd === 1) {
        add('m4t', 'Planowanie sprintu', at(day, 10, 0), 90, {
          location: 'Sala Wisła',
          description:
            'Cel sprintu i podział zadań.\n\nPrzed spotkaniem przejrzyj tablicę i oszacuj swoje zgłoszenia. ' +
            'Zabierz listę ryzyk z poprzedniego sprintu.',
        });
      }
      if (wd === 3) add('m4t', 'Przegląd architektury', at(day, 13, 0), 60);
      if (wd === 5 && week % 2 === 0) add('m4t', 'Retrospektywa', at(day, 14, 0), 60);
      if (wd === 2 || wd === 4) add('itens', 'Spotkanie z klientem', at(day, 15, 0), 60, { location: 'Teams' });
      if (wd === 4 && week % 2 === 1) add('itens', 'Warsztat produktowy', at(day, 11, 0), 120);
    }
    if (wd === 1 || wd === 3 || wd === 5) add('private', 'Siłownia', at(day, 18, 0), 90, { location: 'Calypso' });
    if (wd === 6 && week % 2 === 0) add('private', 'Rower', at(day, 10, 0), 180);
    if (wd === 0) add('private', 'Obiad u rodziców', at(day, 14, 0), 150);
  }
  const tomorrow = new Date(first);
  tomorrow.setDate(first.getDate() + 22);
  list.push({
    id: 'mock-birthday',
    calendarId: 'private',
    title: 'Urodziny Ani',
    start: iso(at(tomorrow, 0)),
    end: iso(at(new Date(tomorrow.getTime() + 86_400_000), 0)),
    allDay: true,
    calendar: cal.private.name,
    color: cal.private.color,
  });
  // All-day: a three-day leave starting tomorrow and a single day later this week.
  const allDay = (calendarId: string, title: string, fromDay: number, days: number) => {
    const start = new Date(now);
    start.setHours(0, 0, 0, 0);
    start.setDate(start.getDate() + fromDay);
    const end = new Date(start);
    end.setDate(start.getDate() + days);
    add(calendarId, title, start, 0, { end: iso(end), allDay: true });
  };
  allDay('private', 'Urlop w górach', 1, 3);
  allDay('m4t', 'Dzień bez spotkań', 2, 1);
  allDay('itens', 'Wdrożenie u klienta', 2, 1);

  // Relative to now, so the summary always has something running and something next.
  add('itens', 'Konsultacje', new Date(now.getTime() - 20 * 60_000), 60);
  add('private', 'Odbiór paczki', new Date(now.getTime() + 85 * 60_000), 30);
  return list;
}

/** ?printer=idle|finished|offline|error shows the printer in that state instead of printing. */
function printerState(): Printer['state'] {
  const forced = new URLSearchParams(location.search).get('printer');
  return forced === 'idle' || forced === 'finished' || forced === 'offline' || forced === 'error' ? forced : 'printing';
}

export function mockState(): DashboardState {
  const sunrise = new Date();
  sunrise.setHours(7, 11, 0, 0);
  const sunset = new Date();
  sunset.setHours(15, 32, 0, 0);
  return {
    weather: {
      updated: iso(new Date()),
      place: 'Warszawa',
      now: { temp: 4.6, feelsLike: 1.2, code: 3, isDay: true, windKmh: 14, gustKmh: 31, windDir: 250, pressure: 1009, humidity: 86 },
      today: {
        min: 1.8,
        max: 6.3,
        sunrise: iso(sunrise),
        sunset: iso(sunset),
        precipProb: 70,
        precipMm: 3.4,
        uvMax: 0.9,
        uvPeak: '12:00',
        daylightMin: 501,
        pressureTrend: 'down',
      },
      hours: hours(),
      days: days(),
    },
    calendars: MOCK_CALENDARS,
    events: events(),
    music: {
      title: 'Midnight City',
      artist: 'M83',
      album: "Hurry Up, We're Dreaming",
      artUrl: null,
      isPlaying: true,
      progressMs: 83_000,
      readAt: Date.now(),
      durationMs: 243_000,
      device: 'Głośnik w salonie',
      volume: 45,
    },
    musicLibrary: {
      playlists: [
        'Do pracy',
        'Wieczór',
        'Rower',
        'Lo-fi',
        'Odkryj w tym tygodniu',
        'Polski rock',
        'Synthwave',
        'Kuchnia',
        'Na drogę',
        'Klasyka',
      ].map((name, i) => ({ name, uri: `spotify:playlist:mock${i}`, imageUrl: null, detail: `${12 + i * 7} utworów` })),
      recent: [
        ['Hurry Up, We’re Dreaming', 'M83'],
        ['Random Access Memories', 'Daft Punk'],
        ['In Rainbows', 'Radiohead'],
        ['Currents', 'Tame Impala'],
      ].map(([name, artist], i) => ({ name, uri: `spotify:album:mock${i}`, imageUrl: null, detail: artist })),
      likedUri: 'spotify:user:mock:collection',
    },
    lights: [
      { id: 'desk-left', name: 'Lampka lewa', kind: 'switch', on: true },
      { id: 'desk-right', name: 'Lampka prawa', kind: 'switch', on: false },
      { id: 'strip', name: 'Listwa LED', kind: 'rgbw', on: true, brightness: 60, color: '#ff7a3d', white: 20 },
    ],
    printer: {
      name: 'Drukarka',
      model: 'X2D',
      state: printerState(),
      job: { name: 'Uchwyt na słuchawki', progress: 62, remainingMin: 47, layer: 148, totalLayers: 240, thumbnailUrl: null },
      nozzles: [
        { temp: 219, target: 220 },
        { temp: 31, target: 0 },
      ],
      bed: { temp: 65, target: 65 },
      chamber: 31,
      ams: [
        { color: '#f95d73', material: 'PETG', remaining: null, active: false },
        { color: '#d3c5a3', material: 'PETG', remaining: null, active: true },
        { color: '#161616', material: 'PETG', remaining: null, active: false },
        { color: null, material: '', remaining: null, active: false },
      ],
      amsHumidity: 16,
      message: printerState() === 'error' ? 'Komunikat drukarki HMS_0300_2000_0001_0001' : null,
    },
    photos: null, // generated asynchronously, see mockPhotos.ts
    display: { brightness: 80 },
  };
}

// Dashboard state: loaded from /api/state and kept current by /api/events (SSE).
// With ?mock in the URL, or in development without a server, it uses test data, so the look
// can be worked on alone. The built page on the Pi never does: there the server may still
// be starting, so it keeps trying. Fine-grained $state means a weather update repaints only
// the weather card.
import { mockState } from './mock';
import { mockPhotos } from './mockPhotos';
import type {
  CalendarSource,
  DashboardState,
  Display,
  Light,
  MusicLibrary,
  NowPlaying,
  Photos,
  Printer,
  SpotifyDevice,
  Update,
} from './types';

export const dash: DashboardState = $state({
  weather: null,
  calendars: null,
  events: null,
  music: null,
  lights: [],
  printer: null,
  photos: null,
  display: null,
  musicLibrary: null,
});

export const connection = $state({ mode: 'connecting' as 'connecting' | 'live' | 'mock' | 'offline' });

// Generated landscapes stand in for Immich while the server has no photos (until stage D3).
let placeholderPhotos: Photos | null = null;

function setPhotos(photos: Photos | null): void {
  dash.photos = photos ?? placeholderPhotos;
}

function apply(update: Update): void {
  switch (update.topic) {
    case 'weather':
      dash.weather = update.data as DashboardState['weather'];
      break;
    case 'calendars':
      dash.calendars = update.data as CalendarSource[] | null;
      break;
    case 'events':
      dash.events = update.data as DashboardState['events'];
      break;
    case 'music':
      dash.music = update.data as NowPlaying | null;
      break;
    case 'lights':
      dash.lights = update.data as Light[];
      break;
    case 'printer':
      dash.printer = update.data as Printer | null;
      break;
    case 'photos':
      setPhotos(update.data as Photos | null);
      break;
    case 'display':
      dash.display = update.data as Display | null;
      break;
    case 'musicLibrary':
      dash.musicLibrary = update.data as MusicLibrary | null;
      break;
  }
}

function useMock(): void {
  Object.assign(dash, mockState());
  connection.mode = 'mock';
  void mockPhotos().then((photos) => {
    dash.photos = photos;
  });
}

const RETRY_MS = 2000;

export async function connect(): Promise<void> {
  if (new URLSearchParams(location.search).has('mock')) {
    useMock();
    return;
  }
  for (;;) {
    try {
      const res = await fetch('/api/state');
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const state = (await res.json()) as DashboardState;
      Object.assign(dash, state);
      setPhotos(state.photos);
      break;
    } catch (e) {
      if (import.meta.env.DEV) {
        console.warn('No dashboard server, using test data:', e);
        useMock();
        return;
      }
      connection.mode = 'offline';
      await new Promise((r) => setTimeout(r, RETRY_MS));
    }
  }
  void mockPhotos(true).then((photos) => {
    placeholderPhotos = photos;
    if (!dash.photos) dash.photos = photos;
  });
  const events = new EventSource('/api/events');
  events.addEventListener('update', (ev) => apply(JSON.parse((ev as MessageEvent<string>).data) as Update));
  events.onopen = () => (connection.mode = 'live');
  events.onerror = () => (connection.mode = 'offline'); // EventSource reconnects by itself
}

async function post(path: string, body?: unknown): Promise<void> {
  if (connection.mode === 'mock') return;
  await fetch(path, {
    method: 'POST',
    headers: body ? { 'Content-Type': 'application/json' } : undefined,
    body: body ? JSON.stringify(body) : undefined,
  });
}

export function setLight(id: string, change: Partial<Pick<Light, 'on' | 'brightness' | 'color' | 'white'>>): void {
  const light = dash.lights.find((l) => l.id === id);
  if (light) Object.assign(light, change); // show it at once; the server confirms over SSE
  void post(`/api/lights/${encodeURIComponent(id)}`, change);
}

export function music(action: 'play' | 'pause' | 'next' | 'previous'): void {
  const m = dash.music;
  if (m && (action === 'play' || action === 'pause')) {
    m.progressMs += m.isPlaying ? Date.now() - m.readAt : 0;
    m.readAt = Date.now();
    m.isPlaying = action === 'play';
  }
  void post(`/api/music/${action}`);
}

/** Plays a playlist or album; on `deviceId` when nothing is active yet. */
export function playContext(uri: string, deviceId?: string): void {
  void post('/api/music/context', deviceId ? { uri, deviceId } : { uri });
}

/** Moves the playback to another Spotify device. */
export function transferTo(deviceId: string): void {
  void post('/api/music/device', { id: deviceId });
}

/** Volume of the playing device, 0-100. */
export function setVolume(percent: number): void {
  if (dash.music) dash.music.volume = percent;
  void post('/api/music/volume', { percent });
}

/** The Spotify devices to choose from; fetched when the picker opens. */
export async function fetchDevices(): Promise<SpotifyDevice[]> {
  if (connection.mode === 'mock') {
    return [
      { id: 'mock-speaker', name: 'Głośnik w salonie', type: 'Speaker', isActive: true, volume: 45 },
      { id: 'mock-pc', name: 'fedora', type: 'Computer', isActive: false, volume: 80 },
      { id: 'mock-phone', name: 'iPhone', type: 'Smartphone', isActive: false, volume: 100 },
    ];
  }
  try {
    const res = await fetch('/api/music/devices');
    return res.ok ? ((await res.json()) as SpotifyDevice[]) : [];
  } catch {
    return [];
  }
}

/** Backlight brightness of the screen, 0-100; the server writes it to /sys/class/backlight. */
export function setBrightness(brightness: number): void {
  dash.display = { brightness };
  void post('/api/display', { brightness });
}

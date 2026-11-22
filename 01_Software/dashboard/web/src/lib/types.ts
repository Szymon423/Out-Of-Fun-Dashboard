// Data shared with dashboard-server: /api/state returns DashboardState and /api/events
// sends Update. The server must use exactly these field names (docs/plan.md).

export interface WeatherNow {
  temp: number; // °C
  feelsLike: number; // °C
  code: number; // WMO weather code, as given by Open-Meteo
  isDay: boolean;
  windKmh: number;
  humidity: number; // %
}

export interface WeatherHour {
  time: string; // ISO 8601, local time of the Pi
  temp: number;
  code: number;
  isDay: boolean;
  precipProb: number; // %
}

export interface WeatherDay {
  date: string; // YYYY-MM-DD, local
  code: number;
  min: number;
  max: number;
  precipProb: number; // %
}

export interface Weather {
  updated: string; // ISO 8601
  place: string;
  now: WeatherNow;
  today: { min: number; max: number; sunrise: string; sunset: string; precipProb: number };
  hours: WeatherHour[]; // the next hours, starting with the coming full hour
  days: WeatherDay[]; // the next days, starting tomorrow
}

/** One calendar source (an account), e.g. private, M4T, Itens. */
export interface CalendarSource {
  id: string;
  name: string;
  color: string; // CSS colour used for its events
}

export interface CalendarEvent {
  id: string;
  calendarId: string; // CalendarSource.id
  title: string;
  start: string; // ISO 8601
  end: string;
  allDay: boolean;
  calendar: string; // name of the source, shown under the title
  color: string; // CSS colour of the source
  location?: string;
}

export interface NowPlaying {
  title: string;
  artist: string;
  album: string;
  artUrl: string | null;
  isPlaying: boolean;
  progressMs: number; // position when the server read it ...
  readAt: number; // ... at this time, epoch ms, so the UI can move on by itself
  durationMs: number;
  device: string;
}

/** A desk light: a plain on/off lamp, or the RGBW LED strip. */
export interface Light {
  id: string;
  name: string;
  kind: 'switch' | 'rgbw';
  on: boolean;
  brightness?: number; // 0-100, rgbw only
  color?: string; // #rrggbb, rgbw only
  white?: number; // 0-100, the W channel, rgbw only
}

/** The screen itself; brightness is the backlight, set by the server. */
export interface Display {
  brightness: number; // 0-100
}

export interface PrinterJob {
  name: string;
  progress: number; // 0-100
  remainingMin: number;
  layer: number;
  totalLayers: number;
  thumbnailUrl: string | null;
}

export interface AmsTray {
  color: string; // CSS colour of the filament
  material: string; // PLA, PETG, ...
  remaining: number | null; // %, null when the AMS cannot tell
  active: boolean;
}

/** Bambu Lab printer, read over MQTT by the server. */
export interface Printer {
  name: string;
  model: string;
  state: 'offline' | 'idle' | 'printing' | 'paused' | 'finished' | 'error';
  job: PrinterJob | null;
  nozzle: { temp: number; target: number };
  bed: { temp: number; target: number };
  chamber: number | null;
  ams: AmsTray[];
  message: string | null; // error or attention text from the printer
}

/** A photo from Immich, served (resized and cached) by the server. */
export interface Photo {
  id: string;
  url: string; // screen-sized image
  /** The same photo heavily blurred and small (e.g. from Immich's thumbhash); stretched to the
   *  screen it gives the frosted glass behind the screensaver clock without a live blur. */
  blurUrl: string | null;
  takenAt: string; // ISO 8601
  place: string | null;
}

export interface Photos {
  album: string;
  items: Photo[];
  /** Immich memory ("N lat temu"), shown on the summary page. */
  memory: { title: string; photo: Photo } | null;
}

export interface DashboardState {
  weather: Weather | null;
  calendars: CalendarSource[] | null;
  events: CalendarEvent[] | null;
  music: NowPlaying | null;
  lights: Light[];
  printer: Printer | null;
  photos: Photos | null;
  display: Display | null;
}

export type Topic = 'weather' | 'calendars' | 'events' | 'music' | 'lights' | 'printer' | 'photos' | 'display';
export interface Update {
  topic: Topic;
  data: unknown;
}

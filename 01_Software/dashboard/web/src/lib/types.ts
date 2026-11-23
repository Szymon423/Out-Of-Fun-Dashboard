// Data shared with dashboard-server: /api/state returns DashboardState and /api/events
// sends Update. The server must use exactly these field names (docs/plan.md).

export interface WeatherNow {
  temp: number; // °C
  feelsLike: number; // °C
  code: number; // WMO weather code, as given by Open-Meteo
  isDay: boolean;
  windKmh: number;
  gustKmh: number;
  windDir: number; // degrees the wind comes from, 0 = north
  pressure: number; // hPa, at sea level
  humidity: number; // %
}

export interface WeatherHour {
  time: string; // ISO 8601, local time of the Pi
  temp: number;
  code: number;
  isDay: boolean;
  precipProb: number; // %
  precipMm: number; // in that hour
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
  today: {
    min: number;
    max: number;
    sunrise: string;
    sunset: string;
    precipProb: number; // %
    precipMm: number; // the day's total
    uvMax: number;
    uvPeak: string | null; // "HH:MM" of the strongest sun, null when it stays low
    daylightMin: number;
    pressureTrend: 'up' | 'down' | 'steady'; // over the next three hours
  };
  hours: WeatherHour[]; // the next hours, starting with the coming full hour
  days: WeatherDay[]; // the next days, starting tomorrow
}

/** One calendar source (an account), e.g. private, work, a client. */
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
  description?: string; // plain text; Outlook's Teams footer already cut off by the server
}

export interface NowPlaying {
  title: string;
  uri: string; // spotify:track:..., to mark the track in a list
  artist: string;
  album: string;
  artUrl: string | null;
  isPlaying: boolean;
  progressMs: number; // position when the server read it ...
  readAt: number; // ... at this time, epoch ms, so the UI can move on by itself
  durationMs: number;
  device: string;
  volume: number | null; // of the device, 0-100; null when it cannot be set
}

/** A playlist, an album or the liked songs, playable by its spotify: URI. */
export interface LibraryItem {
  name: string;
  uri: string;
  imageUrl: string | null;
  detail: string; // "22 utwory", or the artist of an album
}

/** The Spotify library for the music page, refreshed by the server every 30 min. */
export interface MusicLibrary {
  playlists: LibraryItem[];
  recent: LibraryItem[]; // albums played lately
  likedUri: string | null;
}

/** A track of a playlist or album, from GET /api/music/tracks. */
export interface Track {
  name: string;
  artist: string;
  uri: string;
  durationMs: number;
}

/** A Spotify Connect device, from GET /api/music/devices. */
export interface SpotifyDevice {
  id: string;
  name: string;
  type: string; // Computer, Smartphone, Speaker, TV, ...
  isActive: boolean;
  volume: number | null;
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
  color: string | null; // CSS colour of the filament; null for an empty slot
  material: string; // PLA, PETG, ...; empty for an empty slot
  remaining: number | null; // %, null when the AMS cannot tell
  active: boolean;
}

/** Bambu Lab printer, read over MQTT by the server. */
export interface Temperature {
  temp: number; // °C, whole degrees
  target: number; // 0 when not heating
}

export interface Printer {
  name: string;
  model: string; // e.g. X2D
  state: 'offline' | 'idle' | 'printing' | 'paused' | 'finished' | 'error';
  job: PrinterJob | null;
  nozzles: Temperature[]; // one per extruder; the X2D has two
  bed: Temperature;
  chamber: number | null;
  ams: AmsTray[];
  amsHumidity: number | null; // %, of the first AMS unit
  message: string | null; // error or attention text from the printer
}

/** A photo from Immich, served (resized and cached) by the server. */
export interface Photo {
  id: string;
  url: string; // screen-sized image, /api/photos/<id>.jpg
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
  musicLibrary: MusicLibrary | null;
}

export type Topic =
  | 'weather'
  | 'calendars'
  | 'events'
  | 'music'
  | 'lights'
  | 'printer'
  | 'photos'
  | 'display'
  | 'musicLibrary';
export interface Update {
  topic: Topic;
  data: unknown;
}

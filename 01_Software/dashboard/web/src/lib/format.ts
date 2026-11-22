// Polish formatting of times, dates and weather codes.
import type { Component } from 'svelte';
import Cloud from '@lucide/svelte/icons/cloud';
import CloudDrizzle from '@lucide/svelte/icons/cloud-drizzle';
import CloudFog from '@lucide/svelte/icons/cloud-fog';
import CloudHail from '@lucide/svelte/icons/cloud-hail';
import CloudLightning from '@lucide/svelte/icons/cloud-lightning';
import CloudMoon from '@lucide/svelte/icons/cloud-moon';
import CloudRain from '@lucide/svelte/icons/cloud-rain';
import CloudSnow from '@lucide/svelte/icons/cloud-snow';
import CloudSun from '@lucide/svelte/icons/cloud-sun';
import Cloudy from '@lucide/svelte/icons/cloudy';
import Moon from '@lucide/svelte/icons/moon';
import Sun from '@lucide/svelte/icons/sun';

const timeFmt = new Intl.DateTimeFormat('pl-PL', { hour: '2-digit', minute: '2-digit' });
const hourFmt = new Intl.DateTimeFormat('pl-PL', { hour: '2-digit' });
const dateFmt = new Intl.DateTimeFormat('pl-PL', { weekday: 'long', day: 'numeric', month: 'long' });
const weekdayFmt = new Intl.DateTimeFormat('pl-PL', { weekday: 'short' });

export const hhmm = (d: Date | string): string => timeFmt.format(new Date(d));
export const hourOnly = (d: Date | string): string => hourFmt.format(new Date(d));

export function longDate(d: Date): string {
  const s = dateFmt.format(d);
  return s.charAt(0).toUpperCase() + s.slice(1);
}

/** "pon.", "wt." for a local YYYY-MM-DD date. */
export function weekdayShort(date: string): string {
  const [y, m, d] = date.split('-').map(Number);
  return weekdayFmt.format(new Date(y, m - 1, d));
}

export function duration(ms: number): string {
  const s = Math.max(0, Math.floor(ms / 1000));
  return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
}

/** "za 25 min", "za 2 h 10 min", "teraz" for an event start relative to now. */
export function startsIn(start: Date, now: Date): string {
  const min = Math.round((start.getTime() - now.getTime()) / 60_000);
  if (min <= 0) return 'teraz';
  if (min < 60) return `za ${min} min`;
  const h = Math.floor(min / 60);
  const rest = min % 60;
  return rest ? `za ${h} h ${rest} min` : `za ${h} h`;
}

export const temp = (t: number): string => `${Math.round(t)}°`;

export type Tone = 'sun' | 'night' | 'cloud' | 'rain' | 'snow' | 'storm' | 'fog';

export interface WeatherLook {
  label: string;
  icon: Component;
  tone: Tone;
}

/** WMO weather code (Open-Meteo) to a Polish label, an icon and a colour tone. */
export function weatherLook(code: number, isDay: boolean): WeatherLook {
  const clear = isDay ? Sun : Moon;
  const partly = isDay ? CloudSun : CloudMoon;
  const sky: Tone = isDay ? 'sun' : 'night';
  if (code === 0) return { label: isDay ? 'Słonecznie' : 'Bezchmurnie', icon: clear, tone: sky };
  if (code === 1) return { label: isDay ? 'Przeważnie słonecznie' : 'Przeważnie bezchmurnie', icon: partly, tone: sky };
  if (code === 2) return { label: 'Częściowe zachmurzenie', icon: partly, tone: 'cloud' };
  if (code === 3) return { label: 'Pochmurno', icon: Cloudy, tone: 'cloud' };
  if (code === 45 || code === 48) return { label: 'Mgła', icon: CloudFog, tone: 'fog' };
  if (code >= 51 && code <= 55) return { label: 'Mżawka', icon: CloudDrizzle, tone: 'rain' };
  if (code === 56 || code === 57) return { label: 'Marznąca mżawka', icon: CloudDrizzle, tone: 'rain' };
  if (code === 61) return { label: 'Słaby deszcz', icon: CloudRain, tone: 'rain' };
  if (code === 63) return { label: 'Deszcz', icon: CloudRain, tone: 'rain' };
  if (code === 65) return { label: 'Ulewa', icon: CloudRain, tone: 'rain' };
  if (code === 66 || code === 67) return { label: 'Marznący deszcz', icon: CloudRain, tone: 'rain' };
  if (code === 71) return { label: 'Słaby śnieg', icon: CloudSnow, tone: 'snow' };
  if (code === 73) return { label: 'Śnieg', icon: CloudSnow, tone: 'snow' };
  if (code === 75) return { label: 'Intensywny śnieg', icon: CloudSnow, tone: 'snow' };
  if (code === 77) return { label: 'Ziarna śniegu', icon: CloudSnow, tone: 'snow' };
  if (code >= 80 && code <= 82) return { label: code === 82 ? 'Gwałtowne ulewy' : 'Przelotny deszcz', icon: CloudRain, tone: 'rain' };
  if (code === 85 || code === 86) return { label: 'Przelotny śnieg', icon: CloudSnow, tone: 'snow' };
  if (code === 95) return { label: 'Burza', icon: CloudLightning, tone: 'storm' };
  if (code === 96 || code === 99) return { label: 'Burza z gradem', icon: CloudHail, tone: 'storm' };
  return { label: 'Brak danych', icon: Cloud, tone: 'cloud' };
}

/** A shared clock that ticks once per minute, on the minute (no per-second DOM work). */
export function minuteClock(onTick: (now: Date) => void): () => void {
  let timer = 0;
  const schedule = () => {
    const now = new Date();
    onTick(now);
    timer = window.setTimeout(schedule, 60_000 - (now.getSeconds() * 1000 + now.getMilliseconds()) + 20);
  };
  schedule();
  return () => clearTimeout(timer);
}

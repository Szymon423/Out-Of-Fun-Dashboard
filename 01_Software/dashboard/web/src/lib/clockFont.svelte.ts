// Candidate fonts for the big clock: tall, thin digits in the spirit of Apple's lock screen.
// ?fonts shows them side by side on the real screen; a tap picks one (stored in this browser).
// Once the choice is made, keep that one and drop the other font packages.
import '@fontsource-variable/inter-tight';
import '@fontsource-variable/sofia-sans-extra-condensed';
import '@fontsource-variable/roboto-flex/full.css';
import '@fontsource-variable/antonio';
import '@fontsource-variable/big-shoulders-display';
import '@fontsource-variable/smooch-sans';

export interface ClockFont {
  id: string;
  label: string;
  /** Inline style for the digits; `scale` evens out the visual height between fonts. */
  style: string;
  scale: number;
}

export const CLOCK_FONTS: ClockFont[] = [
  { id: 'inter', label: 'Inter Thin', style: "font-family:'Inter Variable';font-weight:120;letter-spacing:-0.055em", scale: 1 },
  { id: 'inter-tight', label: 'Inter Tight Thin', style: "font-family:'Inter Tight Variable';font-weight:110;letter-spacing:-0.045em", scale: 1.02 },
  { id: 'sofia', label: 'Sofia Sans Extra Condensed', style: "font-family:'Sofia Sans Extra Condensed Variable';font-weight:150;letter-spacing:-0.035em;font-variant-numeric:normal", scale: 1.32 },
  { id: 'roboto-flex', label: 'Roboto Flex, wąski', style: "font-family:'Roboto Flex Variable';font-weight:130;font-stretch:55%;letter-spacing:-0.02em;font-variant-numeric:normal", scale: 1.18 },
  { id: 'antonio', label: 'Antonio', style: "font-family:'Antonio Variable';font-weight:110;letter-spacing:-0.02em;font-variant-numeric:normal", scale: 1.2 },
  { id: 'big-shoulders', label: 'Big Shoulders Display', style: "font-family:'Big Shoulders Display Variable';font-weight:130;letter-spacing:-0.01em;font-variant-numeric:normal", scale: 1.25 },
  { id: 'smooch', label: 'Smooch Sans', style: "font-family:'Smooch Sans Variable';font-weight:160;letter-spacing:-0.01em;font-variant-numeric:normal", scale: 1.35 },
];

const KEY = 'dashboard.clockFont';

function stored(): string {
  try {
    return localStorage.getItem(KEY) ?? 'sofia';
  } catch {
    return 'sofia';
  }
}

export const clockFont = $state({ id: stored() });

export function chooseClockFont(id: string): void {
  clockFont.id = id;
  try {
    localStorage.setItem(KEY, id);
  } catch {
    // no storage (private mode): the choice lasts until the page reloads
  }
}

export function currentClockFont(): ClockFont {
  return CLOCK_FONTS.find((f) => f.id === clockFont.id) ?? CLOCK_FONTS[0];
}

/** Style for clock digits of the given base size in px. */
export function clockStyle(sizePx: number): string {
  const f = currentClockFont();
  return `${f.style};font-size:${Math.round(sizePx * f.scale)}px`;
}

// Colour helpers for the RGBW strip: hue <-> #rrggbb at full saturation.

export function hueToHex(hue: number): string {
  const f = (n: number) => {
    const k = (n + hue / 60) % 6;
    const v = 1 - Math.max(0, Math.min(k, 4 - k, 1));
    return Math.round(v * 255)
      .toString(16)
      .padStart(2, '0');
  };
  return `#${f(5)}${f(3)}${f(1)}`;
}

export function hexToHue(hex: string): number {
  const n = parseInt(hex.slice(1), 16);
  const r = ((n >> 16) & 255) / 255;
  const g = ((n >> 8) & 255) / 255;
  const b = (n & 255) / 255;
  const max = Math.max(r, g, b);
  const d = max - Math.min(r, g, b);
  if (d === 0) return 0;
  let h = max === r ? ((g - b) / d) % 6 : max === g ? (b - r) / d + 2 : (r - g) / d + 4;
  h *= 60;
  return Math.round(h < 0 ? h + 360 : h);
}

export const HUE_TRACK =
  'linear-gradient(90deg, #ff0000, #ffff00 17%, #00ff00 33%, #00ffff 50%, #0000ff 67%, #ff00ff 83%, #ff0000)';

/** Presets for the strip: [label, colour, white channel %]. */
export const LED_PRESETS: [string, string, number][] = [
  ['Ciepła biel', '#ff9d4d', 80],
  ['Neutralna', '#ffd2a8', 100],
  ['Zachód', '#ff5a1f', 10],
  ['Róż', '#ff2d78', 0],
  ['Fiolet', '#8a3dff', 0],
  ['Ocean', '#1f6bff', 0],
  ['Turkus', '#00d1c1', 0],
  ['Zieleń', '#2fd158', 0],
];

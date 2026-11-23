<!--
  The next 24 hours as one chart, readable from across the desk: the temperature as a curve
  starting at "Teraz", the weather icon and the temperature every three hours, the chance of
  rain as bars along the bottom (with the amount when it rains). Day and night show in the
  curve itself: warm in the day, the cool lavender of the moon icons at night. (Shaded grey
  night bands were tried first; they read as boxes, not as night.)
  Shapes are SVG, text and icons are HTML on top (crisp, in the page's fonts). It changes
  only with a new forecast, every 15 minutes, so it is painted once and then left alone.
-->
<script lang="ts">
  import { hourOnly, weatherLook } from '../../format';
  import type { WeatherHour, WeatherNow } from '../../types';

  let { now, hours }: { now: WeatherNow; hours: WeatherHour[] } = $props();

  const STEP = 3; // labels and icons every third hour
  const TOP = 72; // the curve's band starts below the icons and the labels

  // The chart fills whatever height its card leaves it; the rows follow from that.
  let width = $state(800);
  let height = $state(300);
  const BAR_BASE = $derived(height - 34); // the rain bars stand on this line, hours below it
  const BAR_MAX = $derived(Math.min(64, height * 0.18));
  const CURVE_BOTTOM = $derived(BAR_BASE - BAR_MAX - 30);

  // Point 0 is now, then one per hour.
  const points = $derived([
    { temp: now.temp, code: now.code, isDay: now.isDay, precipProb: 0, precipMm: 0, label: 'Teraz' },
    ...hours.slice(0, 24).map((h) => ({ ...h, label: hourOnly(h.time) })),
  ]);

  const x = (i: number) => 26 + (i * (width - 52)) / Math.max(1, points.length - 1);
  const lo = $derived(Math.min(...points.map((p) => p.temp)) - 1);
  const hi = $derived(Math.max(...points.map((p) => p.temp)) + 1);
  const y = (t: number) => CURVE_BOTTOM - ((t - lo) / Math.max(1, hi - lo)) * (CURVE_BOTTOM - TOP);

  /** A smooth line through the points (Catmull-Rom as cubic Béziers). */
  const curve = $derived.by(() => {
    const pts = points.map((p, i) => [x(i), y(p.temp)]);
    let d = `M${pts[0][0]},${pts[0][1]}`;
    for (let i = 0; i < pts.length - 1; i++) {
      const [x0, y0] = pts[Math.max(0, i - 1)];
      const [x1, y1] = pts[i];
      const [x2, y2] = pts[i + 1];
      const [x3, y3] = pts[Math.min(pts.length - 1, i + 2)];
      d += ` C${x1 + (x2 - x0) / 6},${y1 + (y2 - y0) / 6} ${x2 - (x3 - x1) / 6},${y2 - (y3 - y1) / 6} ${x2},${y2}`;
    }
    return d;
  });
  const area = $derived(`${curve} L${x(points.length - 1)},${CURVE_BOTTOM + 20} L${x(0)},${CURVE_BOTTOM + 20} Z`);

  const half = $derived((width - 52) / Math.max(1, points.length - 1) / 2);

  const DAY = 'var(--accent)';
  const NIGHT = 'var(--tone-night)';
  const tint = (isDay: boolean) => (isDay ? DAY : NIGHT);

  /** Gradient stops along the width: the colour turns halfway between a day and a night hour. */
  const stops = $derived.by(() => {
    const out: { offset: number; color: string }[] = [{ offset: 0, color: tint(points[0].isDay) }];
    for (let i = 1; i < points.length; i++) {
      if (points[i].isDay === points[i - 1].isDay) continue;
      const at = (x(i - 1) + x(i)) / 2 / width;
      out.push({ offset: at, color: tint(points[i - 1].isDay) }, { offset: at, color: tint(points[i].isDay) });
    }
    out.push({ offset: 1, color: tint(points[points.length - 1].isDay) });
    return out;
  });
  const shown = $derived(points.map((p, i) => ({ ...p, i, look: weatherLook(p.code, p.isDay) })).filter((p) => p.i % STEP === 0));
</script>

<div class="chart" bind:clientWidth={width} bind:clientHeight={height}>
  <svg {width} {height} viewBox="0 0 {width} {height}" aria-hidden="true">
    <defs>
      <linearGradient id="hc-tint" gradientUnits="userSpaceOnUse" x1="0" y1="0" x2={width} y2="0">
        {#each stops as stop, i (i)}
          <stop offset={stop.offset} stop-color={stop.color} />
        {/each}
      </linearGradient>
      <!-- the area under the curve fades out downwards -->
      <linearGradient id="hc-fade" x1="0" y1="0" x2="0" y2="1">
        <stop offset="0" stop-color="#fff" stop-opacity="0.3" />
        <stop offset="1" stop-color="#fff" stop-opacity="0" />
      </linearGradient>
      <mask id="hc-mask" maskUnits="userSpaceOnUse" x="0" y="0" {width} {height}>
        <rect x="0" y={TOP - 10} {width} height={CURVE_BOTTOM + 30 - TOP} fill="url(#hc-fade)" />
      </mask>
    </defs>

    <path d={area} fill="url(#hc-tint)" mask="url(#hc-mask)" />
    <path d={curve} fill="none" stroke="url(#hc-tint)" stroke-width="3" stroke-linecap="round" />

    {#each points as p, i (i)}
      {#if i > 0}
        {@const h = (p.precipProb / 100) * BAR_MAX}
        <rect x={x(i) - half * 0.55} y={BAR_BASE - h} width={half * 1.1} height={h} rx="2" fill="var(--tone-rain)" opacity="0.65" />
      {/if}
    {/each}
    <line x1="0" x2={width} y1={BAR_BASE} y2={BAR_BASE} stroke="var(--line)" />

    {#each shown as p (p.i)}
      <circle cx={x(p.i)} cy={y(p.temp)} r={p.i === 0 ? 6 : 4.5} fill="var(--card-solid)" stroke={tint(p.isDay)} stroke-width="3" />
    {/each}
  </svg>

  {#each shown as p (p.i)}
    <div class="tick tone-{p.look.tone}" style:left="{x(p.i)}px">
      <span class="icon"><p.look.icon size={26} strokeWidth={1.7} /></span>
    </div>
    <span class="temp num" class:now={p.i === 0} style:left="{x(p.i)}px" style:top="{y(p.temp) - 34}px">
      {Math.round(p.temp)}°
    </span>
    {#if p.i > 0 && p.precipProb >= 20}
      <span class="rain num" style:left="{x(p.i)}px" style:top="{BAR_BASE - (p.precipProb / 100) * BAR_MAX - 20}px">
        {p.precipMm >= 0.1 ? `${p.precipMm.toFixed(1).replace('.', ',')} mm` : `${p.precipProb}%`}
      </span>
    {/if}
    <span class="hour num" class:now={p.i === 0} style:left="{x(p.i)}px" style:top="{BAR_BASE + 8}px">{p.label}</span>
  {/each}
</div>

<style>
  .chart {
    position: relative;
    flex: 1;
    min-height: 220px;
    margin-top: 14px;
  }

  svg {
    position: absolute;
    inset: 0;
  }

  .tick,
  .temp,
  .rain,
  .hour {
    position: absolute;
    transform: translateX(-50%);
    white-space: nowrap;
  }

  .tick {
    top: 0;
  }

  .icon {
    color: var(--tone);
  }

  .temp {
    font-size: 19px;
    font-weight: 600;
  }

  .temp.now {
    font-size: 21px;
    font-weight: 700;
  }

  .rain {
    font-size: 12px;
    font-weight: 600;
    color: var(--cool-text);
  }

  .hour {
    font-size: 14px;
    color: var(--muted);
  }

  .hour.now {
    font-weight: 700;
    color: var(--text);
  }
</style>

<!--
  Weather. The main card: the weather now, then the next 24 hours as a large chart (the most
  useful part, readable from a distance). Under it five tiles with the details of the day, as
  in Apple Weather: UV, wind, pressure, sun and rain. On the right, quieter, the next seven days.
-->
<script lang="ts">
  import Droplets from '@lucide/svelte/icons/droplets';
  import Gauge from '@lucide/svelte/icons/gauge';
  import Navigation2 from '@lucide/svelte/icons/navigation-2';
  import Sun from '@lucide/svelte/icons/sun';
  import ArrowDown from '@lucide/svelte/icons/arrow-down';
  import ArrowUp from '@lucide/svelte/icons/arrow-up';
  import Sunrise from '@lucide/svelte/icons/sunrise';
  import Umbrella from '@lucide/svelte/icons/umbrella';
  import Wind from '@lucide/svelte/icons/wind';
  import PageFrame from '../components/PageFrame.svelte';
  import HourlyChart from '../components/weather/HourlyChart.svelte';
  import { hhmm, temp, weatherLook, weekdayShort } from '../format';
  import { dash } from '../state.svelte';

  const w = $derived(dash.weather);
  const look = $derived(w ? weatherLook(w.now.code, w.now.isDay) : null);
  const days = $derived(w?.days ?? []);
  const lo = $derived(Math.min(...days.map((d) => d.min)));
  const hi = $derived(Math.max(...days.map((d) => d.max)));
  const span = $derived(Math.max(1, hi - lo));

  function uvLevel(uv: number): string {
    if (uv < 3) return 'niski';
    if (uv < 6) return 'umiarkowany';
    if (uv < 8) return 'wysoki';
    if (uv < 11) return 'bardzo wysoki';
    return 'ekstremalny';
  }

  /** Where the wind comes from, in words, from degrees (0 = north). */
  function windFrom(deg: number): string {
    const names = ['północy', 'płn.-wsch.', 'wschodu', 'płd.-wsch.', 'południa', 'płd.-zach.', 'zachodu', 'płn.-zach.'];
    const name = names[Math.round((((deg % 360) + 360) % 360) / 45) % 8];
    return name === 'wschodu' ? 'ze wschodu' : `z ${name}`;
  }

  const TREND = { up: 'rośnie', down: 'spada', steady: 'stabilne' } as const;

  function daylight(min: number): string {
    return `dzień ${Math.floor(min / 60)} h ${String(min % 60).padStart(2, '0')} min`;
  }

  const mm = (v: number) => v.toFixed(1).replace('.', ',');
</script>

<PageFrame title="Pogoda" subtitle={w?.place ?? ''}>
  {#if w && look}
    <div class="layout">
      <section class="main card">
        <header class="now tone-{look.tone}">
          <span class="icon"><look.icon size={52} strokeWidth={1.5} /></span>
          <span class="t num">{temp(w.now.temp)}</span>
          <div class="desc">
            <b>{look.label}</b>
            <span class="num">odczuwalna {temp(w.now.feelsLike)} · max {temp(w.today.max)} · min {temp(w.today.min)}</span>
          </div>
          <span class="updated num">akt. {hhmm(w.updated)}</span>
        </header>
        <HourlyChart now={w.now} hours={w.hours} />
      </section>

      <section class="details">
        <div class="tile card">
          <div class="label"><Sun size={15} /> UV</div>
          <div class="value num">{Math.round(w.today.uvMax)}</div>
          <div class="caption">{uvLevel(w.today.uvMax)}{w.today.uvPeak ? `, najwyżej o ${w.today.uvPeak}` : ''}</div>
        </div>
        <div class="tile card">
          <div class="label"><Wind size={15} /> Wiatr</div>
          <div class="value num">
            {Math.round(w.now.windKmh)}<small> km/h</small>
            <span class="arrow" style:transform="rotate({w.now.windDir + 180}deg)"><Navigation2 size={18} fill="currentColor" /></span>
          </div>
          <div class="caption num">porywy {Math.round(w.now.gustKmh)} km/h, {windFrom(w.now.windDir)}</div>
        </div>
        <div class="tile card">
          <div class="label"><Gauge size={15} /> Ciśnienie</div>
          <div class="value num">{w.now.pressure}<small> hPa</small></div>
          <div class="caption">{TREND[w.today.pressureTrend]}</div>
        </div>
        <div class="tile card">
          <div class="label"><Sunrise size={15} /> Słońce</div>
          <div class="value num sun">
            <span><ArrowUp size={16} strokeWidth={2.4} />{hhmm(w.today.sunrise)}</span>
            <span><ArrowDown size={16} strokeWidth={2.4} />{hhmm(w.today.sunset)}</span>
          </div>
          <div class="caption num">{daylight(w.today.daylightMin)}</div>
        </div>
        <div class="tile card">
          <div class="label"><Umbrella size={15} /> Opady</div>
          <div class="value num">{mm(w.today.precipMm)}<small> mm</small></div>
          <div class="caption num">dziś, szansa {w.today.precipProb}%</div>
        </div>
      </section>

      <section class="days card">
        <div class="label">7 dni</div>
        {#each days as d (d.date)}
          {@const dl = weatherLook(d.code, true)}
          <div class="day tone-{dl.tone}">
            <span class="name">{weekdayShort(d.date)}</span>
            <span class="dicon"><dl.icon size={20} strokeWidth={1.7} /></span>
            <span class="rain num" class:show={d.precipProb >= 30}><Droplets size={11} />{d.precipProb}%</span>
            <span class="min num">{temp(d.min)}</span>
            <span class="range"><i style:left="{((d.min - lo) / span) * 100}%" style:right="{((hi - d.max) / span) * 100}%"></i></span>
            <span class="max num">{temp(d.max)}</span>
          </div>
        {/each}
      </section>
    </div>
  {:else}
    <p class="empty">Czekam na prognozę…</p>
  {/if}
</PageFrame>

<style>
  .layout {
    display: grid;
    grid-template-columns: 1fr 318px;
    grid-template-rows: 1fr 142px;
    grid-template-areas:
      'main days'
      'details days';
    gap: var(--gap);
    height: 100%;
  }

  .card {
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
  }

  .main {
    grid-area: main;
    display: flex;
    flex-direction: column;
    padding: 18px 24px 10px;
    min-height: 0;
  }

  .now {
    display: flex;
    align-items: center;
    gap: 14px;
  }

  .now .icon {
    color: var(--tone);
  }

  .t {
    font-size: 58px;
    font-weight: 300;
    line-height: 1;
    letter-spacing: -0.03em;
  }

  .desc {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .desc b {
    font-size: 20px;
    font-weight: 650;
  }

  .desc span {
    font-size: 15px;
    color: var(--muted);
  }

  .updated {
    margin-left: auto;
    align-self: flex-start;
    font-size: 13px;
    color: var(--faint);
  }

  .details {
    grid-area: details;
    display: grid;
    grid-template-columns: repeat(5, 1fr);
    gap: 14px;
  }

  .tile {
    display: flex;
    flex-direction: column;
    padding: 14px 16px;
    min-width: 0;
  }

  .label {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 12.5px;
    font-weight: 650;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--faint);
  }

  .value {
    display: flex;
    align-items: baseline;
    gap: 2px;
    margin-top: 10px;
    font-size: 30px;
    font-weight: 500;
    line-height: 1.05;
  }

  .value.sun {
    flex-direction: column;
    gap: 0;
    margin-top: 6px;
    font-size: 20px;
  }

  .value.sun span {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  .value.sun :global(svg) {
    color: var(--tone-sun);
  }

  .value small {
    font-size: 15px;
    font-weight: 500;
    color: var(--muted);
  }

  .arrow {
    display: inline-flex;
    align-self: center;
    margin-left: auto;
    color: var(--cool-text);
  }

  .caption {
    margin-top: auto;
    font-size: 13.5px;
    line-height: 1.3;
    color: var(--muted);
  }

  /* Seven days, quieter than the hours: smaller type, muted. */
  .days {
    grid-area: days;
    display: flex;
    flex-direction: column;
    padding: 14px 20px;
  }

  .days .label {
    margin-bottom: 4px;
  }

  .day {
    flex: 1;
    display: grid;
    grid-template-columns: 38px 24px 44px 30px 1fr 30px;
    align-items: center;
    gap: 8px;
    border-bottom: 1px solid var(--line);
    color: var(--tone);
  }

  .day:last-child {
    border-bottom: 0;
  }

  .name {
    font-size: 15px;
    font-weight: 600;
    color: var(--text);
  }

  .rain {
    display: inline-flex;
    align-items: center;
    gap: 2px;
    font-size: 12px;
    color: var(--cool-text);
    opacity: 0;
  }

  .rain.show {
    opacity: 1;
  }

  .min {
    font-size: 15px;
    color: var(--muted);
    text-align: right;
  }

  .max {
    font-size: 15px;
    font-weight: 600;
    color: var(--text);
  }

  .range {
    position: relative;
    height: 5px;
    border-radius: 3px;
    background: var(--fill-strong);
  }

  .range i {
    position: absolute;
    top: 0;
    bottom: 0;
    border-radius: 3px;
    background: linear-gradient(90deg, var(--tone-rain), var(--tone-sun));
  }

  .empty {
    color: var(--faint);
  }
</style>

<!-- Current weather, today's range and the next hours (Open-Meteo through the server). -->
<script lang="ts">
  import Droplets from '@lucide/svelte/icons/droplets';
  import Sunrise from '@lucide/svelte/icons/sunrise';
  import Sunset from '@lucide/svelte/icons/sunset';
  import Thermometer from '@lucide/svelte/icons/thermometer';
  import Umbrella from '@lucide/svelte/icons/umbrella';
  import Wind from '@lucide/svelte/icons/wind';
  import { dash } from '../state.svelte';
  import { hhmm, hourOnly, temp, weatherLook, weekdayShort } from '../format';
  import Card from './Card.svelte';

  const HOURS = 8;

  const w = $derived(dash.weather);
  const look = $derived(w ? weatherLook(w.now.code, w.now.isDay) : null);
  const hours = $derived(
    w ? w.hours.slice(0, HOURS).map((h) => ({ ...h, look: weatherLook(h.code, h.isDay) })) : [],
  );
  const days = $derived(w ? w.days.slice(0, 3).map((d) => ({ ...d, look: weatherLook(d.code, true) })) : []);
</script>

<Card title={w?.place ?? 'Pogoda'} icon={Thermometer} class="weather">
  {#snippet aside()}
    {#if w}<span class="num">akt. {hhmm(w.updated)}</span>{/if}
  {/snippet}

  {#if w && look}
    <div class="now tone-{look.tone}">
      <div class="icon"><look.icon size={84} strokeWidth={1.3} /></div>
      <div class="temp num">{temp(w.now.temp)}</div>
      <div class="desc">
        <div class="label">{look.label}</div>
        <div class="range num">
          <span>max {temp(w.today.max)}</span>
          <span>min {temp(w.today.min)}</span>
        </div>
      </div>
    </div>

    <div class="stats num">
      <span><Thermometer size={15} /> odczuwalna {temp(w.now.feelsLike)}</span>
      <span><Wind size={15} /> {Math.round(w.now.windKmh)} km/h</span>
      <span><Droplets size={15} /> {Math.round(w.now.humidity)}%</span>
      <span class:wet={w.today.precipProb >= 40}><Umbrella size={15} /> {Math.round(w.today.precipProb)}%</span>
    </div>

    <div class="hours">
      {#each hours as h (h.time)}
        <div class="hour tone-{h.look.tone}">
          <span class="h num">{hourOnly(h.time)}</span>
          <h.look.icon size={26} strokeWidth={1.7} />
          <span class="t num">{temp(h.temp)}</span>
          <span class="p num" class:show={h.precipProb >= 30}>{Math.round(h.precipProb)}%</span>
        </div>
      {/each}
    </div>

    <div class="days">
      {#each days as d (d.date)}
        <div class="day tone-{d.look.tone}">
          <span class="name">{weekdayShort(d.date)}</span>
          <d.look.icon size={24} strokeWidth={1.7} />
          <span class="range num"><b>{temp(d.max)}</b> {temp(d.min)}</span>
        </div>
      {/each}
    </div>

    <div class="sun num">
      <span><Sunrise size={15} /> {hhmm(w.today.sunrise)}</span>
      <span><Sunset size={15} /> {hhmm(w.today.sunset)}</span>
    </div>
  {:else}
    <div class="empty">Czekam na prognozę…</div>
  {/if}
</Card>

<style>
  :global(.card.weather) {
    flex: 1;
  }

  .now {
    display: flex;
    align-items: center;
    gap: 18px;
    margin-top: -2px;
  }

  .icon {
    display: grid;
    place-items: center;
    width: 96px;
    height: 96px;
    color: var(--tone);
  }

  .temp {
    font-size: 86px;
    font-weight: 250;
    line-height: 1;
    letter-spacing: -0.04em;
  }

  .desc {
    display: flex;
    flex-direction: column;
    gap: 6px;
    margin-left: 4px;
  }

  .label {
    font-size: 21px;
    font-weight: 550;
  }

  .range {
    display: flex;
    gap: 14px;
    font-size: 15px;
    color: var(--muted);
  }

  .stats {
    display: flex;
    flex-wrap: wrap;
    gap: 8px 18px;
    margin: 16px 0 18px;
    font-size: 14.5px;
    color: var(--muted);
  }

  .stats span {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  .stats .wet {
    color: var(--cool-text);
  }

  .hours {
    display: grid;
    grid-template-columns: repeat(8, 1fr);
    padding: 14px 4px 12px;
    border-top: 1px solid var(--line);
    border-bottom: 1px solid var(--line);
  }

  .hour {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 7px;
    color: var(--tone);
  }

  .hour .h {
    font-size: 13px;
    color: var(--faint);
  }

  .hour .t {
    font-size: 17px;
    font-weight: 550;
    color: var(--text);
  }

  .hour .p {
    font-size: 12px;
    color: var(--cool-text);
    opacity: 0;
  }

  .hour .p.show {
    opacity: 1;
  }

  .days {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 10px;
    padding-top: 14px;
  }

  .day {
    display: flex;
    align-items: center;
    gap: 7px;
    min-width: 0;
    padding: 9px 11px;
    border-radius: 14px;
    background: var(--fill);
    color: var(--tone);
  }

  .day .name {
    width: 28px;
    font-size: 13.5px;
    font-weight: 600;
    color: var(--muted);
  }

  .day .range {
    margin-left: auto;
    font-size: 14.5px;
    color: var(--faint);
  }

  .day .range b {
    font-weight: 600;
    color: var(--text);
  }

  .sun {
    display: flex;
    gap: 22px;
    margin-top: auto;
    padding-top: 14px;
    font-size: 14px;
    color: var(--faint);
  }

  .sun span {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  .empty {
    margin: auto;
    color: var(--faint);
  }

  .tone-sun {
    --tone: var(--tone-sun);
  }
  .tone-night {
    --tone: var(--tone-night);
  }
  .tone-cloud {
    --tone: var(--tone-cloud);
  }
  .tone-rain {
    --tone: var(--tone-rain);
  }
  .tone-snow {
    --tone: var(--tone-snow);
  }
  .tone-storm {
    --tone: var(--tone-storm);
  }
  .tone-fog {
    --tone: var(--tone-fog);
  }
</style>

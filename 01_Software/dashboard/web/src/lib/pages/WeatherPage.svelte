<!-- Weather: now and the next hours on the left, the next seven days on the right. -->
<script lang="ts">
  import Droplets from '@lucide/svelte/icons/droplets';
  import PageFrame from '../components/PageFrame.svelte';
  import WeatherCard from '../components/WeatherCard.svelte';
  import { temp, weatherLook, weekdayShort } from '../format';
  import { dash } from '../state.svelte';

  const days = $derived(dash.weather?.days ?? []);
  const lo = $derived(Math.min(...days.map((d) => d.min)));
  const hi = $derived(Math.max(...days.map((d) => d.max)));
  const span = $derived(Math.max(1, hi - lo));
</script>

<PageFrame title="Pogoda" subtitle={dash.weather?.place ?? ''}>
  <div class="layout">
    <div class="now"><WeatherCard /></div>
    <section class="days">
      {#each days as d (d.date)}
        {@const look = weatherLook(d.code, true)}
        <div class="day tone-{look.tone}">
          <span class="name">{weekdayShort(d.date)}</span>
          <span class="icon"><look.icon size={26} strokeWidth={1.7} /></span>
          <span class="rain num" class:show={d.precipProb >= 30}><Droplets size={13} />{d.precipProb}%</span>
          <span class="min num">{temp(d.min)}</span>
          <span class="range"><i style:left="{((d.min - lo) / span) * 100}%" style:right="{((hi - d.max) / span) * 100}%"></i></span>
          <span class="max num">{temp(d.max)}</span>
        </div>
      {/each}
    </section>
  </div>
</PageFrame>

<style>
  .layout {
    display: grid;
    grid-template-columns: 500px 1fr;
    gap: var(--gap);
    height: 100%;
  }

  .now {
    display: grid;
  }

  .days {
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    padding: 16px 26px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
  }

  .day {
    display: grid;
    grid-template-columns: 54px 40px 62px 40px 1fr 40px;
    align-items: center;
    gap: 10px;
    height: 62px;
    border-bottom: 1px solid var(--line);
    color: var(--tone);
  }

  .day:last-child {
    border-bottom: 0;
  }

  .name {
    font-size: 18px;
    font-weight: 600;
    color: var(--text);
  }

  .rain {
    display: inline-flex;
    align-items: center;
    gap: 3px;
    font-size: 13.5px;
    color: var(--cool-text);
    opacity: 0;
  }

  .rain.show {
    opacity: 1;
  }

  .min {
    font-size: 18px;
    color: var(--muted);
    text-align: right;
  }

  .max {
    font-size: 18px;
    font-weight: 600;
    color: var(--text);
  }

  .range {
    position: relative;
    height: 6px;
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

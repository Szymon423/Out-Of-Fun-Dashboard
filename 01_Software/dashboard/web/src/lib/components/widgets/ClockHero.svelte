<!-- Big clock, date and the weather in one line, like the iPhone lock screen. Tapping the
     weather line opens the weather page. Text changes once a minute. -->
<script lang="ts">
  import { clock } from '../../clock.svelte';
  import { clockStyle } from '../../clockFont.svelte';
  import { hhmm, longDate, temp, weatherLook } from '../../format';
  import { go } from '../../nav.svelte';
  import { dash } from '../../state.svelte';

  let { size = 252 }: { size?: number } = $props();

  const w = $derived(dash.weather);
  const look = $derived(w ? weatherLook(w.now.code, w.now.isDay) : null);
</script>

<div class="hero">
  <div class="date">{longDate(clock.now)}</div>
  <div class="time num" style={clockStyle(size)}>{hhmm(clock.now)}</div>
  {#if w && look}
    <button class="weather tone-{look.tone}" onclick={() => go('weather')}>
      <look.icon size={30} strokeWidth={1.6} />
      <span class="t num">{temp(w.now.temp)}</span>
      <span class="label">{look.label}</span>
      <span class="range num">{temp(w.today.max)} / {temp(w.today.min)}</span>
    </button>
  {/if}
</div>

<style>
  .hero {
    display: flex;
    flex-direction: column;
  }

  .date {
    font-size: 24px;
    font-weight: 560;
    color: var(--muted);
    letter-spacing: -0.01em;
  }

  .time {
    line-height: 0.9;
    margin: 6px 0 0 -4px;
    color: var(--text);
  }

  .weather {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    align-self: flex-start;
    margin-top: 18px;
    padding: 8px 16px 8px 12px;
    border-radius: 999px;
    background: var(--fill);
    color: var(--tone);
  }

  .weather .t {
    font-size: 22px;
    font-weight: 600;
    color: var(--text);
  }

  .weather .label {
    font-size: 17px;
    font-weight: 500;
    color: var(--text);
  }

  .weather .range {
    font-size: 15px;
    color: var(--muted);
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

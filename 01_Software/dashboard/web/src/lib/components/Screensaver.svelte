<!--
  Screensaver in the spirit of Apple TV and the Nest Hub: Immich photos full screen with
  crossfade and a slow zoom, at any time of day. A thin clock with the date and the weather
  sits in the bottom left corner, where the photo place and date sit on the right, both on a
  soft shade along the bottom edge; the photo stays the main thing. (A big frosted panel
  with the clock was dropped on 2026-11-23: it covered too much of the photo.)
  A touch closes it. `covered` turns true once it fully covers the screen, so the pages
  underneath can stop being drawn.
-->
<script lang="ts">
  import { fade } from 'svelte/transition';
  import { clock } from '../clock.svelte';
  import { clockStyle } from '../clockFont.svelte';
  import { hhmm, longDate, temp, weatherLook } from '../format';
  import { closeSaver, nav } from '../nav.svelte';
  import { dash } from '../state.svelte';
  import Slideshow from './Slideshow.svelte';

  let { covered = $bindable(false) }: { covered?: boolean } = $props();

  const SLIDE_MS = 20000;
  const monthYear = new Intl.DateTimeFormat('pl-PL', { month: 'long', year: 'numeric' });

  const photos = $derived(dash.photos?.items ?? []);
  const w = $derived(dash.weather);
  const look = $derived(w ? weatherLook(w.now.code, w.now.isDay) : null);

  let current = $state(0);
  const photo = $derived(photos.length ? photos[current % photos.length] : null);

  function close(e: PointerEvent): void {
    e.stopPropagation();
    closeSaver();
  }
</script>

{#if nav.saver}
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div
    class="saver"
    onpointerdown={close}
    transition:fade={{ duration: 900 }}
    onintroend={() => (covered = true)}
    onoutrostart={() => (covered = false)}
  >
    {#if photos.length}<Slideshow {photos} intervalMs={SLIDE_MS} bind:current />{/if}
    <div class="shade"></div>

    <div class="clock">
      <div class="time num" style={clockStyle(150)}>{hhmm(clock.now)}</div>
      <div class="line num">
        {longDate(clock.now)}
        {#if w && look}
          <span class="dot">·</span>
          <look.icon size={24} strokeWidth={1.7} />
          {temp(w.now.temp)}
        {/if}
      </div>
    </div>

    {#if photo}
      {#key photo.id}
        <div class="about" in:fade={{ duration: 1200, delay: 400 }} out:fade={{ duration: 600 }}>
          {#if photo.place}<div class="place">{photo.place}</div>{/if}
          <div class="when">{monthYear.format(new Date(photo.takenAt))}</div>
        </div>
      {/key}
    {/if}
  </div>
{/if}

<style>
  .saver {
    position: absolute;
    inset: 0;
    z-index: 10;
    background: #000;
    color: #fff;
    overflow: hidden;
    will-change: opacity; /* painted as a layer from the start, not when the fade-out begins */
  }

  /* Only the bottom edge is darkened, enough for white text on a bright photo. */
  .shade {
    position: absolute;
    inset: 0;
    background: linear-gradient(to top, rgba(0, 0, 0, 0.55), rgba(0, 0, 0, 0.22) 20%, transparent 42%);
  }

  .clock,
  .about {
    position: absolute;
    bottom: 40px;
    text-shadow: 0 1px 10px rgba(0, 0, 0, 0.3);
  }

  .clock {
    left: 48px;
  }

  .time {
    line-height: 0.9;
    margin-left: -4px;
  }

  .line {
    display: flex;
    align-items: center;
    gap: 9px;
    margin-top: 12px;
    font-size: 23px;
    font-weight: 520;
    opacity: 0.95;
  }

  .dot {
    opacity: 0.6;
  }

  .about {
    right: 48px;
    text-align: right;
  }

  .place {
    font-size: 21px;
    font-weight: 620;
  }

  .when {
    margin-top: 3px;
    font-size: 17px;
    opacity: 0.78;
  }
</style>

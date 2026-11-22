<!--
  Screensaver in the spirit of iPhone StandBy: Immich photos with crossfade and a slow zoom,
  at any time of day, with a big thin clock and the weather on a frosted panel. (A night look
  with only a dim clock on black was dropped on 2026-11-22: the user wants the photos always.)
  A touch closes it.
  `covered` turns true once it fully covers the screen, so the pages underneath can stop
  being drawn.
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

  const photos = $derived(dash.photos?.items ?? []);
  const w = $derived(dash.weather);
  const look = $derived(w ? weatherLook(w.now.code, w.now.isDay) : null);

  let current = $state(0);
  const photo = $derived(photos.length ? photos[current % photos.length] : null);

  // The frosted panel behind the clock shows the blurred copy of the current photo, placed
  // where the photo is on screen, so the panel looks like glass over it.
  const SLIDE_MS = 20000;
  const PANEL_LEFT = 40;
  const PANEL_BOTTOM = 40;
  let panelHeight = $state(0);

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
    <div class="scrim"></div>
    <div
      class="panel"
      style:left="{PANEL_LEFT}px"
      style:bottom="{PANEL_BOTTOM}px"
      style:--zoom-ms="{SLIDE_MS + 2500}ms"
      bind:clientHeight={panelHeight}
    >
      {#if photo}
        {#key photo.id}
          <img
            class="glass"
            class:even={current % 2 === 0}
            src={photo.blurUrl ?? photo.url}
            class:live-blur={!photo.blurUrl}
            alt=""
            style:left="{-PANEL_LEFT}px"
            style:top="{panelHeight + PANEL_BOTTOM - 720}px"
            in:fade={{ duration: 1600 }}
            out:fade={{ duration: 1600 }}
          />
        {/key}
      {/if}
      <i class="tint"></i>
      <div class="content">
        <div class="date">{longDate(clock.now)}</div>
        <div class="time num" style={clockStyle(230)}>{hhmm(clock.now)}</div>
        {#if w && look}
          <div class="weather num">
            <look.icon size={28} strokeWidth={1.6} />
            {temp(w.now.temp)} · {look.label}
          </div>
        {/if}
      </div>
    </div>
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

  .scrim {
    position: absolute;
    inset: 0;
    background:
      linear-gradient(to top, rgba(0, 0, 0, 0.6), transparent 55%),
      linear-gradient(to right, rgba(0, 0, 0, 0.35), transparent 60%);
  }

  /* Frosted glass: the blurred photo (no live filter), a light tint and a bright edge. */
  .panel {
    position: absolute;
    border-radius: 38px;
    overflow: hidden;
    box-shadow:
      0 0 0 1px rgba(255, 255, 255, 0.22) inset,
      0 24px 70px rgba(0, 0, 0, 0.35);
  }

  .glass {
    position: absolute;
    width: 1280px;
    height: 720px;
    object-fit: cover;
    will-change: transform, opacity;
    transform-origin: 30% 40%;
    animation: zoom var(--zoom-ms) ease-out forwards;
  }

  .glass.even {
    transform-origin: 70% 60%;
  }

  /* Without a ready blurred copy (a server that does not send one yet), blur the photo live. */
  .glass.live-blur {
    filter: blur(40px);
  }

  .tint {
    position: absolute;
    inset: 0;
    background: linear-gradient(160deg, rgba(255, 255, 255, 0.12), rgba(0, 0, 0, 0.28));
  }

  .content {
    position: relative;
    padding: 26px 40px 30px 34px;
  }

  .content .date {
    font-size: 28px;
    font-weight: 560;
    opacity: 0.95;
  }

  @keyframes zoom {
    from {
      transform: scale(1);
    }
    to {
      transform: scale(1.08);
    }
  }

  .time {
    line-height: 0.88;
    margin-left: -6px;
  }

  .content .weather {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-top: 14px;
    font-size: 22px;
    font-weight: 500;
    opacity: 0.92;
  }
</style>

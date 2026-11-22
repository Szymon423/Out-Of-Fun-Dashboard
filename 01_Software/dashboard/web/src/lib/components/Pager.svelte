<!--
  The pages side by side on one track; a swipe moves the track, so only the page coming into
  view gets painted. A pull down from the top edge brings the control bar. A swipe up (or a
  double tap) opens the overview, a swipe down closes it. The bar at the bottom hints at it.
  Overview, made to stay smooth on the Pi: the current page, already painted, fades back as
  one layer, and light tiles take its place. Picking a tile jumps the track there without
  sliding through the pages in between, so only the target page is painted.
-->
<script lang="ts">
  import type { Component } from 'svelte';
  import { pagerGestures } from '../gestures';
  import { go, nav, PAGES, toggleOverview, type PageId } from '../nav.svelte';
  import CalendarPage from '../pages/CalendarPage.svelte';
  import HomePage from '../pages/HomePage.svelte';
  import MusicPage from '../pages/MusicPage.svelte';
  import PhotosPage from '../pages/PhotosPage.svelte';
  import PrinterPage from '../pages/PrinterPage.svelte';
  import SummaryPage from '../pages/SummaryPage.svelte';
  import WeatherPage from '../pages/WeatherPage.svelte';
  import Overview from './Overview.svelte';

  const W = 1280;
  const PULL_ZONE_PX = 110; // a pull down must start this close to the top edge
  const BAR_TRAVEL_PX = 140;

  const VIEW: Record<PageId, Component> = {
    home: HomePage,
    printer: PrinterPage,
    calendar: CalendarPage,
    summary: SummaryPage,
    weather: WeatherPage,
    music: MusicPage,
    photos: PhotosPage,
  };

  let drag = $state(0);
  let dragging = $state(false);
  // No sliding animation while the overview is open or just closing: the track jumps.
  let instant = $state(false);

  $effect(() => {
    if (nav.overview) {
      instant = true;
      return;
    }
    const timer = setTimeout(() => (instant = false), 600);
    return () => clearTimeout(timer);
  });

  const offset = $derived.by(() => {
    let dx = drag;
    if ((nav.index === 0 && dx > 0) || (nav.index === PAGES.length - 1 && dx < 0)) dx *= 0.3;
    return -nav.index * W + dx;
  });

  const gestures = pagerGestures({
    canSwipe: () => !nav.overview && !nav.controls,
    onDrag(dx) {
      dragging = true;
      drag = dx;
    },
    onRelease(dx, vx) {
      dragging = false;
      drag = 0;
      const step = dx < -W * 0.18 || vx < -0.45 ? 1 : dx > W * 0.18 || vx > 0.45 ? -1 : 0;
      if (step) go(nav.index + step);
    },
    canPull: (startY, dy) => !nav.overview && !nav.controls && dy > 0 && startY < PULL_ZONE_PX,
    onPull(dy) {
      nav.pull = dy;
    },
    onPullEnd(dy, vy) {
      nav.pull = null;
      nav.controls = dy > BAR_TRAVEL_PX * 0.45 || vy > 0.4;
    },
    canSwipeVertical: (direction) => (direction === 'up' ? !nav.overview && !nav.controls : nav.overview),
    onSwipeVertical: (direction) => toggleOverview(direction === 'up'),
    onDoubleTap: () => toggleOverview(),
  });
</script>

<div class="pager" class:dragging {@attach gestures}>
  <div class="stage" class:away={nav.overview}>
    <div class="track" class:instant style:transform="translate3d({offset}px, 0, 0)">
      {#each PAGES as page, i (page.id)}
        {@const View = VIEW[page.id]}
        <section class="page" style:left="{i * W}px" aria-hidden={i !== nav.index}>
          <View />
        </section>
      {/each}
    </div>
  </div>
  <Overview />
  <i class="home-indicator" class:hidden={nav.overview}></i>
</div>

<style>
  .pager {
    position: absolute;
    inset: 0;
    overflow: hidden;
    touch-action: none;
  }

  .stage {
    position: absolute;
    inset: 0;
    will-change: transform, opacity;
    transition:
      opacity 0.3s ease,
      transform 0.42s var(--ease);
  }

  .stage.away {
    opacity: 0;
    transform: scale(0.9);
    pointer-events: none;
  }

  .track {
    position: absolute;
    left: 0;
    top: 0;
    width: 1280px;
    height: 720px;
    will-change: transform;
    transition: transform 0.45s var(--ease);
  }

  .track.instant,
  .pager.dragging .track {
    transition: none;
  }

  .page {
    position: absolute;
    top: 0;
    width: 1280px;
    height: 720px;
  }

  /* iOS-style hint at the bottom: swipe up from here for the overview. */
  .home-indicator {
    position: absolute;
    left: 50%;
    bottom: 7px;
    width: 150px;
    height: 5px;
    margin-left: -75px;
    border-radius: 3px;
    background: var(--faint);
    opacity: 0.7;
    transition: opacity 0.25s ease;
    pointer-events: none;
  }

  .home-indicator.hidden {
    opacity: 0;
  }
</style>

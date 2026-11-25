<!--
  Overview of the pages as light tiles (icon, name, one live line), like an app switcher. The
  summary sits in the middle as a large tile with the clock; the pages left of it in the pager
  (home, printer, calendar) stand in a column on its left, those right of it on its right, in
  the order a swipe goes through them.
  Cheap on purpose: a few small elements fading and scaling in, instead of seven live pages,
  which the Pi cannot paint at once without a stutter. It stays in the page all the time and
  only CSS transitions show it, so opening it creates no elements. A tap opens the page; a swipe down or
  a double tap on an empty spot closes it. Taps in the first moment after opening are ignored:
  the finger that opened the overview must not pick a tile by accident.
-->
<script lang="ts">
  import { visibleEvents } from '../calendarView.svelte';
  import { clock } from '../clock.svelte';
  import { clockStyle } from '../clockFont.svelte';
  import { hhmm, longDate, temp, weatherLook } from '../format';
  import { go, nav, PAGES, type PageId } from '../nav.svelte';
  import { PRINTER_STATE, remaining } from '../printer';
  import { currentPhoto } from '../photoClock.svelte';
  import { dash } from '../state.svelte';

  const COLOR: Record<PageId, string> = {
    home: '#ff9f0a',
    printer: '#30d158',
    calendar: '#ff453a',
    summary: '#0a84ff',
    weather: '#64d2ff',
    music: '#ff375f',
    photos: '#bf5af2',
  };

  const weekday = new Intl.DateTimeFormat('pl-PL', { weekday: 'long' });

  function info(id: PageId): string {
    switch (id) {
      case 'home': {
        const on = dash.lights.filter((l) => l.on).length;
        return on ? `Włączone ${on} z ${dash.lights.length}` : 'Wszystko wyłączone';
      }
      case 'printer': {
        const p = dash.printer;
        if (!p) return 'Brak połączenia';
        if (p.job && p.state === 'printing') return `${Math.round(p.job.progress)}% · ${remaining(p.job.remainingMin)}`;
        return PRINTER_STATE[p.state].label;
      }
      case 'calendar': {
        const now = clock.now;
        const next = visibleEvents()
          .filter((e) => !e.allDay && new Date(e.end) > now && new Date(e.start).toDateString() === now.toDateString())
          .sort((a, b) => a.start.localeCompare(b.start))[0];
        return next ? `${hhmm(next.start)} ${next.title}` : 'Dziś nic więcej';
      }
      case 'summary':
        return `${hhmm(clock.now)} · ${weekday.format(clock.now)}`;
      case 'weather': {
        const w = dash.weather;
        return w ? `${temp(w.now.temp)} · ${weatherLook(w.now.code, w.now.isDay).label}` : 'Brak prognozy';
      }
      case 'music': {
        const m = dash.music;
        return m ? `${m.title} · ${m.artist}` : 'Nic nie gra';
      }
      case 'photos':
        return dash.photos?.memory?.title ?? dash.photos?.album ?? 'Immich';
    }
  }

  const memory = $derived(currentPhoto()?.url ?? dash.photos?.memory?.photo.url ?? null); // the one shown everywhere

  const SETTLE_MS = 400;
  let openedAt = 0;
  $effect(() => {
    if (nav.overview) openedAt = performance.now();
  });

  function pick(i: number): void {
    if (performance.now() - openedAt < SETTLE_MS) return;
    go(i);
  }
</script>

<div class="overview" class:open={nav.overview} aria-hidden={!nav.overview}>
  <header>
    <h2>Pulpity</h2>
    <span>Przesuń w dół, aby wrócić</span>
  </header>
  <div class="tiles">
    {#each PAGES as page, i (page.id)}
      <button
        class="tile"
        class:center={page.id === 'summary'}
        class:current={i === nav.index}
        style:grid-area={page.id}
        class:photo={page.id === 'photos' && memory}
        style:--c={COLOR[page.id]}
        style:--i={i}
        tabindex={nav.overview ? 0 : -1}
        onclick={() => pick(i)}
      >
        {#if page.id === 'photos' && memory}
          <img src={memory} alt="" />
          <i class="scrim"></i>
        {/if}
        {#if page.id === 'summary'}
          <span class="top">
            <span class="icon"><page.icon size={26} strokeWidth={2} /></span>
            <span class="title">{page.title}</span>
          </span>
          <span class="hero">
            <span class="clock num" style={clockStyle(128)}>{hhmm(clock.now)}</span>
            <span class="date">{longDate(clock.now)}</span>
            {#if dash.weather}
              <span class="info">
                {temp(dash.weather.now.temp)} · {weatherLook(dash.weather.now.code, dash.weather.now.isDay).label}
              </span>
            {/if}
          </span>
        {:else}
          <span class="icon"><page.icon size={26} strokeWidth={2} /></span>
          <span class="text">
            <span class="title">{page.title}</span>
            <span class="info">{info(page.id)}</span>
          </span>
        {/if}
      </button>
    {/each}
  </div>
</div>

<style>
  .overview {
    position: absolute;
    inset: 0;
    display: flex;
    flex-direction: column;
    justify-content: center;
    gap: 34px;
    padding: 0 88px 20px;
    opacity: 0;
    pointer-events: none;
    transition: opacity 0.22s ease;
    will-change: opacity;
  }

  /* Own layers, painted once in advance: an element that only becomes a layer when its
     animation starts gets painted right then, which on the Pi is a 100 ms jerk. */
  header,
  .tile {
    will-change: transform;
  }

  .overview.open {
    opacity: 1;
    pointer-events: auto;
  }

  header {
    display: flex;
    align-items: baseline;
    gap: 16px;
    margin-left: 4px;
  }

  header span {
    font-size: 16px;
    color: var(--muted);
  }

  h2 {
    margin: 0;
    font-size: 34px;
    font-weight: 720;
    letter-spacing: -0.02em;
  }

  .tiles {
    display: grid;
    grid-template-columns: 290px 390px 290px;
    grid-template-rows: repeat(3, 156px);
    grid-template-areas:
      'home summary weather'
      'printer summary music'
      'calendar summary photos';
    justify-content: center;
    gap: 20px 24px;
  }

  .tile {
    position: relative;
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    min-width: 0;
    padding: 20px 20px 18px;
    border-radius: 28px;
    background: var(--card);
    box-shadow: var(--shadow);
    text-align: left;
    overflow: hidden;
    transform: scale(0.92);
    transition: transform 0.32s var(--ease);
  }

  .tile.center {
    padding: 24px 26px 26px;
  }

  .top {
    display: flex;
    align-items: center;
    gap: 14px;
  }

  .hero {
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .hero .clock {
    line-height: 0.9;
    margin: 0 0 10px -4px;
  }

  .hero .date {
    font-size: 19px;
    font-weight: 600;
  }

  .open .tile {
    transform: none;
    transition-delay: calc(40ms + var(--i) * 22ms);
  }

  .tile:active {
    transform: scale(0.96);
  }

  .tile.current {
    box-shadow:
      0 0 0 3px var(--c),
      var(--shadow);
  }

  .tile img {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .scrim {
    position: absolute;
    inset: 0;
    background: linear-gradient(to top, rgba(0, 0, 0, 0.6), rgba(0, 0, 0, 0.05) 70%);
  }

  .icon {
    position: relative;
    display: grid;
    place-items: center;
    width: 52px;
    height: 52px;
    border-radius: 16px;
    background: var(--c);
    color: #fff;
  }

  .text {
    position: relative;
    display: flex;
    flex-direction: column;
    gap: 3px;
    min-width: 0;
  }

  .title {
    font-size: 20px;
    font-weight: 680;
  }

  .info {
    font-size: 14.5px;
    color: var(--muted);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .tile.photo .title,
  .tile.photo .info {
    color: #fff;
  }
</style>

<!--
  The details of a calendar event, opened by a tap on it in the day or week view: calendar,
  title, date, time with how long and how soon, place and the full description.
  Mounted next to the pages, not inside them: the pages block touch scrolling for the swipe
  gestures, and a long description has to scroll. A tap beside the card closes it; so does
  leaving the calendar page or the screensaver starting.
-->
<script lang="ts">
  import CalendarDays from '@lucide/svelte/icons/calendar-days';
  import Clock from '@lucide/svelte/icons/clock';
  import MapPin from '@lucide/svelte/icons/map-pin';
  import X from '@lucide/svelte/icons/x';
  import { fade } from 'svelte/transition';
  import { calView } from '../../calendarView.svelte';
  import { clock } from '../../clock.svelte';
  import { hhmm } from '../../format';
  import { nav, PAGES } from '../../nav.svelte';
  import { dash } from '../../state.svelte';
  import type { CalendarEvent } from '../../types';

  const DAY_MS = 86_400_000;
  const dayFmt = new Intl.DateTimeFormat('pl-PL', { weekday: 'long', day: 'numeric', month: 'long' });
  const shortFmt = new Intl.DateTimeFormat('pl-PL', { day: 'numeric', month: 'long' });

  const ev = $derived(calView.selected);

  // ?event=next opens the next event with a description (or else the next one), for checking.
  if (new URLSearchParams(location.search).get('event') === 'next') {
    $effect(() => {
      const upcoming = (dash.events ?? []).filter((e) => new Date(e.end) > new Date());
      if (calView.selected || !upcoming.length) return;
      calView.selected = upcoming.find((e) => e.description) ?? upcoming[0];
    });
  }

  // Close when the calendar page is left or the screensaver starts.
  $effect(() => {
    if (calView.selected && (PAGES[nav.index]?.id !== 'calendar' || nav.saver || nav.overview)) {
      calView.selected = null;
    }
  });

  function close(): void {
    calView.selected = null;
  }

  function span(min: number): string {
    const h = Math.floor(min / 60);
    const m = Math.round(min % 60);
    if (!h) return `${m} min`;
    return m ? `${h} h ${m} min` : `${h} h`;
  }

  function dateLine(e: CalendarEvent): string {
    const start = new Date(e.start);
    // All-day events end at the midnight after their last day.
    const last = new Date(new Date(e.end).getTime() - (e.allDay ? DAY_MS : 1));
    if (start.toDateString() === last.toDateString()) return dayFmt.format(start);
    return `${shortFmt.format(start)} – ${shortFmt.format(last)}`;
  }

  function timeLine(e: CalendarEvent): string {
    if (e.allDay) return 'Cały dzień';
    const minutes = (new Date(e.end).getTime() - new Date(e.start).getTime()) / 60_000;
    return `${hhmm(e.start)}–${hhmm(e.end)} · ${span(minutes)}`;
  }

  function when(e: CalendarEvent): { text: string; live: boolean } {
    const now = clock.now.getTime();
    const start = new Date(e.start).getTime();
    const end = new Date(e.end).getTime();
    if (now >= end) return { text: 'zakończone', live: false };
    if (now >= start) return { text: e.allDay ? 'dzisiaj' : `trwa, do ${hhmm(e.end)}`, live: true };
    const min = (start - now) / 60_000;
    if (!e.allDay && min < 24 * 60) return { text: `za ${span(Math.max(1, min))}`, live: false };
    const days = Math.round((new Date(start).setHours(0, 0, 0, 0) - new Date(now).setHours(0, 0, 0, 0)) / DAY_MS);
    return { text: days <= 1 ? 'jutro' : `za ${days} dni`, live: false };
  }
</script>

{#if ev}
  {@const status = when(ev)}
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div class="scrim" onpointerdown={(e) => e.target === e.currentTarget && close()} transition:fade={{ duration: 160 }}>
    <div class="sheet" role="dialog" aria-label={ev.title} style:--cal={ev.color}>
      <header>
        <span class="source"><i></i>{ev.calendar}</span>
        <button class="close" aria-label="Zamknij" onclick={close}><X size={20} strokeWidth={2.2} /></button>
      </header>

      <h2>{ev.title}</h2>

      <div class="facts">
        <div class="fact"><CalendarDays size={19} /> <span>{dateLine(ev)}</span></div>
        <div class="fact">
          <Clock size={19} />
          <span class="num">{timeLine(ev)}</span>
          <span class="status" class:live={status.live}>{status.text}</span>
        </div>
        {#if ev.location}
          <div class="fact"><MapPin size={19} /> <span>{ev.location}</span></div>
        {/if}
      </div>

      {#if ev.description}
        <div class="description">{ev.description}</div>
      {/if}
    </div>
  </div>
{/if}

<style>
  .scrim {
    position: absolute;
    inset: 0;
    z-index: 8;
    display: grid;
    place-items: center;
    background: rgba(0, 0, 0, 0.45);
  }

  /* No big blurred shadow (docs/plan.md); the dimmed page sets the card apart. */
  .sheet {
    display: flex;
    flex-direction: column;
    width: 640px;
    max-height: 600px;
    padding: 24px 28px 28px;
    border-radius: 28px;
    border-top: 5px solid var(--cal);
    background: var(--card-solid);
    box-shadow: 0 0 0 1px var(--line);
    animation: rise 0.24s var(--ease);
  }

  @keyframes rise {
    from {
      opacity: 0;
      transform: translateY(14px) scale(0.98);
    }
  }

  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .source {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 14px;
    font-weight: 600;
    color: var(--muted);
  }

  .source i {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--cal);
  }

  .close {
    display: grid;
    place-items: center;
    width: 40px;
    height: 40px;
    margin: -6px -10px 0 0;
    border-radius: 50%;
    color: var(--muted);
    background: var(--fill);
  }

  h2 {
    margin: 10px 0 0;
    font-size: 27px;
    font-weight: 700;
    line-height: 1.2;
    letter-spacing: -0.015em;
  }

  .facts {
    display: flex;
    flex-direction: column;
    gap: 10px;
    margin-top: 18px;
  }

  .fact {
    display: flex;
    align-items: center;
    gap: 12px;
    font-size: 17px;
    color: var(--text);
  }

  .fact :global(svg) {
    flex: none;
    color: var(--muted);
  }

  .fact span:first-of-type {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .status {
    flex: none;
    padding: 2px 10px;
    border-radius: 999px;
    font-size: 14px;
    font-weight: 600;
    color: var(--muted);
    background: var(--fill);
  }

  .status.live {
    color: var(--good);
    background: color-mix(in srgb, var(--good) 16%, transparent);
  }

  .description {
    margin-top: 20px;
    padding-top: 18px;
    border-top: 1px solid var(--line);
    overflow-y: auto;
    font-size: 16px;
    line-height: 1.5;
    color: var(--muted);
    white-space: pre-line;
    overflow-wrap: anywhere;
  }
</style>

<!-- Calendar on the summary, in the manner of Apple's "Up Next": the next five events from now
     on, each with its time, its day when not today, the calendar it comes from and how soon it
     starts. The date itself is left to the clock beside it. Respects the calendar filter of the
     calendar page. A tap opens the calendar page. -->
<script lang="ts">
  import CalendarDays from '@lucide/svelte/icons/calendar-days';
  import { visibleEvents } from '../../calendarView.svelte';
  import { clock } from '../../clock.svelte';
  import { hhmm, startsIn } from '../../format';
  import { dash } from '../../state.svelte';
  import type { CalendarEvent } from '../../types';
  import Widget from '../Widget.svelte';

  const MAX = 5;
  const DAY_MS = 86_400_000;
  const weekdayFmt = new Intl.DateTimeFormat('pl-PL', { weekday: 'short' });
  const dateFmt = new Intl.DateTimeFormat('pl-PL', { day: 'numeric', month: 'short' });

  const upcoming = $derived.by((): CalendarEvent[] => {
    const now = clock.now;
    return visibleEvents()
      .filter((e) => new Date(e.end) > now)
      .sort((a, b) => new Date(a.start).getTime() - new Date(b.start).getTime())
      .slice(0, MAX);
  });

  /** Whole days from today to the event's day (0 today, 1 tomorrow, ...). */
  function daysAway(e: CalendarEvent): number {
    const day = new Date(e.start).setHours(0, 0, 0, 0);
    const today = new Date(clock.now).setHours(0, 0, 0, 0);
    return Math.max(0, Math.round((day - today) / DAY_MS));
  }

  function dayLabel(e: CalendarEvent): string {
    const days = daysAway(e);
    if (days === 0) return 'dziś';
    if (days === 1) return 'jutro';
    if (days < 7) return weekdayFmt.format(new Date(e.start));
    return dateFmt.format(new Date(e.start));
  }

  /** How soon, unless the day column already says it ("jutro" twice reads badly). */
  function relative(e: CalendarEvent): { text: string; kind: 'running' | 'soon' | 'later' } | null {
    const now = clock.now;
    const start = new Date(e.start);
    if (start <= now) return { text: 'trwa', kind: 'running' };
    if (!e.allDay && start.getTime() - now.getTime() < 12 * 3600_000) {
      return { text: startsIn(start, now), kind: 'soon' };
    }
    const days = daysAway(e);
    return days >= 2 ? { text: `za ${days} dni`, kind: 'later' } : null;
  }
</script>

<Widget to="calendar" class="events">
  <div class="head"><CalendarDays size={16} strokeWidth={2.2} /> Najbliższe</div>
  {#if dash.events === null}
    <div class="empty">Łączę z kalendarzem…</div>
  {:else if upcoming.length === 0}
    <div class="empty">Nic w kalendarzu</div>
  {:else}
    <div class="list">
      {#each upcoming as e (e.id)}
        {@const rel = relative(e)}
        <div class="row" style:--cal={e.color}>
          <div class="time num">
            {#if e.allDay}
              <b>{dayLabel(e)}</b>
              <span>cały dzień</span>
            {:else}
              <b>{hhmm(e.start)}</b>
              <span>{dayLabel(e)}</span>
            {/if}
          </div>
          <i class="bar"></i>
          <div class="text">
            <div class="title">{e.title}</div>
            <div class="sub">
              <span class="tag">{e.calendar}</span>
              {#if e.location}<span class="place">{e.location}</span>{/if}
            </div>
          </div>
          {#if rel}<span class="rel num {rel.kind}">{rel.text}</span>{/if}
        </div>
      {/each}
    </div>
  {/if}
</Widget>

<style>
  /* The rows spread over the whole height of the tile. */
  :global(.widget.events) {
    display: flex;
    flex-direction: column;
  }

  .head {
    display: flex;
    align-items: center;
    gap: 7px;
    font-size: 14px;
    font-weight: 650;
    color: var(--red);
  }

  .list {
    display: flex;
    flex: 1;
    flex-direction: column;
    justify-content: space-between;
    margin-top: 12px;
    min-height: 0;
  }

  .row {
    display: flex;
    align-items: center;
    gap: 12px;
    min-height: 0;
  }

  .time {
    display: flex;
    flex-direction: column;
    width: 66px;
    flex: none;
  }

  .time b {
    font-size: 16px;
    font-weight: 650;
    white-space: nowrap;
  }

  .time span {
    font-size: 12.5px;
    color: var(--muted);
    white-space: nowrap;
  }

  .bar {
    align-self: stretch;
    width: 3px;
    flex: none;
    margin: 3px 0;
    border-radius: 2px;
    background: var(--cal);
  }

  .text {
    flex: 1;
    min-width: 0;
  }

  .title {
    font-size: 16px;
    font-weight: 600;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .sub {
    display: flex;
    gap: 8px;
    margin-top: 2px;
    font-size: 12.5px;
    color: var(--muted);
    white-space: nowrap;
    overflow: hidden;
  }

  .tag {
    flex: none;
    font-weight: 650;
    color: var(--cal);
  }

  .place {
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .rel {
    flex: none;
    padding: 3px 9px;
    border-radius: 999px;
    font-size: 12.5px;
    font-weight: 600;
    color: var(--muted);
    background: var(--fill);
  }

  .rel.soon {
    color: var(--accent);
    background: var(--accent-soft);
  }

  .rel.running {
    color: var(--good);
    background: color-mix(in srgb, var(--good) 16%, transparent);
  }

  .empty {
    margin: auto 0;
    color: var(--faint);
    font-size: 16px;
  }
</style>

<!-- Calendar widget in the style of Apple's "Up Next": weekday and day number on the left,
     the next events on the right. A tap opens the calendar page. -->
<script lang="ts">
  import { visibleEvents } from '../../calendarView.svelte';
  import { clock } from '../../clock.svelte';
  import { hhmm, startsIn } from '../../format';
  import { dash } from '../../state.svelte';
  import type { CalendarEvent } from '../../types';
  import Widget from '../Widget.svelte';

  const MAX = 4;
  const weekday = new Intl.DateTimeFormat('pl-PL', { weekday: 'long' });

  const upcoming = $derived.by((): CalendarEvent[] => {
    const now = clock.now;
    const end = new Date(now);
    end.setHours(0, 0, 0, 0);
    end.setDate(end.getDate() + 2); // today and tomorrow
    return visibleEvents()
      .filter((e) => !e.allDay && new Date(e.end) > now && new Date(e.start) < end)
      .sort((a, b) => a.start.localeCompare(b.start))
      .slice(0, MAX);
  });

  function dayLabel(e: CalendarEvent): string {
    const d = new Date(e.start);
    return d.toDateString() === clock.now.toDateString() ? '' : 'jutro ';
  }
</script>

<Widget to="calendar" class="events">
  <div class="layout">
    <div class="day">
      <div class="weekday">{weekday.format(clock.now)}</div>
      <div class="number num">{clock.now.getDate()}</div>
    </div>
    <div class="list">
      {#if dash.events === null}
        <div class="empty">Łączę z kalendarzem…</div>
      {:else if upcoming.length === 0}
        <div class="empty">Dziś i jutro nic więcej</div>
      {:else}
        {#each upcoming as e, i (e.id)}
          {@const running = new Date(e.start) <= clock.now}
          <div class="event" style:--cal={e.color}>
            <i class="bar"></i>
            <div class="text">
              <div class="title">{e.title}</div>
              <div class="when num">
                {dayLabel(e)}{hhmm(e.start)}–{hhmm(e.end)}
                {#if i === 0}
                  <span class="soon" class:running>{running ? 'trwa' : startsIn(new Date(e.start), clock.now)}</span>
                {/if}
              </div>
            </div>
          </div>
        {/each}
      {/if}
    </div>
  </div>
</Widget>

<style>
  .layout {
    display: flex;
    gap: 22px;
    height: 100%;
  }

  .day {
    width: 108px;
    flex: none;
  }

  .weekday {
    font-size: 15px;
    font-weight: 650;
    color: var(--red);
    text-transform: uppercase;
    letter-spacing: 0.04em;
  }

  .number {
    font-size: 64px;
    font-weight: 300;
    line-height: 1;
    margin-top: 4px;
    letter-spacing: -0.03em;
  }

  .list {
    flex: 1;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 14px;
    padding-top: 2px;
  }

  .event {
    display: flex;
    gap: 12px;
  }

  .bar {
    width: 4px;
    flex: none;
    border-radius: 2px;
    background: var(--cal);
  }

  .text {
    min-width: 0;
  }

  .title {
    font-size: 18px;
    font-weight: 600;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .when {
    margin-top: 2px;
    font-size: 14.5px;
    color: var(--muted);
  }

  .soon {
    margin-left: 6px;
    font-weight: 600;
    color: var(--accent);
  }

  .soon.running {
    color: var(--good);
  }

  .empty {
    margin: auto 0;
    color: var(--faint);
    font-size: 16px;
  }
</style>

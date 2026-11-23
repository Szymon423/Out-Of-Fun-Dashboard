<!-- Day and week view: hours 7-23 down, days across, events as blocks in their time slot,
     like Apple Calendar. Overlapping events share the width of the day. A red line marks now.
     All-day events sit in a thin strip under the day names, as bars across the days they
     cover; the strip is left out when there are none. A tap on an event opens its details. -->
<script lang="ts">
  import { clock } from '../../clock.svelte';
  import { eventsOn, sameDay, showDay, startOfDay } from '../../calendarView.svelte';
  import { hhmm } from '../../format';
  import type { CalendarEvent } from '../../types';

  let {
    days,
    events,
    header = true,
    onselect,
  }: { days: Date[]; events: CalendarEvent[]; header?: boolean; onselect?: (event: CalendarEvent) => void } = $props();

  const FIRST_HOUR = 7;
  const LAST_HOUR = 23;
  const HOURS = Array.from({ length: LAST_HOUR - FIRST_HOUR }, (_, i) => FIRST_HOUR + i);
  const weekdayFmt = new Intl.DateTimeFormat('pl-PL', { weekday: 'short' });

  let bodyHeight = $state(460);
  const pxPerMin = $derived(bodyHeight / ((LAST_HOUR - FIRST_HOUR) * 60));

  interface Placed {
    ev: CalendarEvent;
    top: number;
    height: number;
    col: number;
    cols: number;
  }

  /** Timed events of one day, placed in columns so that overlapping ones sit side by side. */
  function place(day: Date): Placed[] {
    const dayStart = startOfDay(day).getTime();
    const minutes = (t: number) => (t - dayStart) / 60_000;
    const lo = FIRST_HOUR * 60;
    const hi = LAST_HOUR * 60;
    const timed = eventsOn(day, events)
      .filter((e) => !e.allDay)
      .map((ev) => ({
        ev,
        from: Math.max(lo, minutes(new Date(ev.start).getTime())),
        to: Math.min(hi, minutes(new Date(ev.end).getTime())),
      }))
      .filter((x) => x.to > lo && x.from < hi)
      .sort((a, b) => a.from - b.from);

    const placed: Placed[] = [];
    let group: { item: (typeof timed)[number]; col: number }[] = [];
    let groupEnd = -1;
    const flush = () => {
      const cols = Math.max(1, ...group.map((g) => g.col + 1));
      for (const g of group) {
        placed.push({
          ev: g.item.ev,
          top: (g.item.from - lo) * pxPerMin,
          height: Math.max(20, (g.item.to - g.item.from) * pxPerMin - 2),
          col: g.col,
          cols,
        });
      }
      group = [];
    };
    for (const item of timed) {
      if (item.from >= groupEnd) {
        flush();
        groupEnd = -1;
      }
      const used = new Set(group.filter((g) => g.item.to > item.from).map((g) => g.col));
      let col = 0;
      while (used.has(col)) col++;
      group.push({ item, col });
      groupEnd = Math.max(groupEnd, item.to);
    }
    flush();
    return placed;
  }

  interface Bar {
    ev: CalendarEvent;
    from: number; // first and last day column it covers
    to: number;
    row: number;
  }

  /** All-day events in rows of bars; beyond the rows that fit, a "+N" per day instead. */
  const allDay = $derived.by(() => {
    const maxRows = days.length === 1 ? 3 : 2;
    const first = startOfDay(days[0]).getTime();
    const last = days.length - 1;
    const index = (t: number) => Math.round((startOfDay(new Date(t)).getTime() - first) / 86_400_000);
    const items = events
      .filter((e) => e.allDay)
      // The end is the midnight after the last day.
      .map((ev) => ({ ev, from: index(new Date(ev.start).getTime()), to: index(new Date(ev.end).getTime() - 1) }))
      .filter((b) => b.to >= 0 && b.from <= last)
      .map((b) => ({ ...b, from: Math.max(0, b.from), to: Math.min(last, b.to) }))
      .sort((a, b) => a.from - b.from || b.to - b.from - (a.to - a.from));

    const rowEnds: number[] = [];
    const placed: Bar[] = [];
    for (const item of items) {
      let row = rowEnds.findIndex((end) => end < item.from);
      if (row < 0) row = rowEnds.push(-1) - 1;
      rowEnds[row] = item.to;
      placed.push({ ...item, row });
    }
    const overflow = rowEnds.length > maxRows;
    const shown = overflow ? maxRows - 1 : rowEnds.length;
    return {
      rows: overflow ? maxRows : rowEnds.length,
      bars: placed.filter((b) => b.row < shown),
      more: days.map((_, i) => placed.filter((b) => b.row >= shown && b.from <= i && b.to >= i).length),
    };
  });

  const nowTop = $derived(((clock.now.getHours() * 60 + clock.now.getMinutes()) - FIRST_HOUR * 60) * pxPerMin);
  const nowVisible = $derived(clock.now.getHours() >= FIRST_HOUR && clock.now.getHours() < LAST_HOUR);
</script>

<div class="grid" style:--days={days.length}>
  {#if header}
    <div class="head">
      <span></span>
      {#each days as d (d.toISOString())}
        <div class="dayhead" class:today={sameDay(d, clock.now)}>
          <span class="wd">{weekdayFmt.format(d)}</span>
          <span class="dn num">{d.getDate()}</span>
        </div>
      {/each}
    </div>
  {/if}

  {#if allDay.rows > 0}
    <div class="allday" style:--rows={allDay.rows}>
      <span class="label">cały<br />dzień</span>
      {#each allDay.bars as b (b.ev.id)}
        <button
          class="bar"
          style:--cal={b.ev.color}
          style:grid-column="{b.from + 2} / {b.to + 3}"
          style:grid-row={b.row + 1}
          onclick={() => onselect?.(b.ev)}
        >
          {b.ev.title}
        </button>
      {/each}
      {#each allDay.more as n, i (i)}
        {#if n > 0}
          <button class="more" style:grid-column={i + 2} style:grid-row={allDay.rows} onclick={() => showDay(days[i])}>
            +{n} więcej
          </button>
        {/if}
      {/each}
    </div>
  {/if}

  <div class="body" bind:clientHeight={bodyHeight}>
    <div class="hours">
      {#each HOURS as h (h)}
        <span class="num" style:top="{(h - FIRST_HOUR) * 60 * pxPerMin}px">{h}:00</span>
      {/each}
    </div>
    {#each days as d (d.toISOString())}
      <div class="col">
        {#each HOURS as h (h)}
          <i class="line" style:top="{(h - FIRST_HOUR) * 60 * pxPerMin}px"></i>
        {/each}
        {#each place(d) as p (p.ev.id)}
          <button
            class="event"
            style:--cal={p.ev.color}
            style:top="{p.top}px"
            style:height="{p.height}px"
            style:left="calc({(p.col / p.cols) * 100}% + 2px)"
            style:width="calc({100 / p.cols}% - 4px)"
            onclick={() => onselect?.(p.ev)}
          >
            <span class="title">{p.ev.title}</span>
            {#if p.height > 34}<span class="time num">{hhmm(p.ev.start)}–{hhmm(p.ev.end)}</span>{/if}
          </button>
        {/each}
        {#if nowVisible && sameDay(d, clock.now)}
          <i class="now" style:top="{nowTop}px"></i>
        {/if}
      </div>
    {/each}
  </div>
</div>

<style>
  .grid {
    display: flex;
    flex-direction: column;
    height: 100%;
    padding: 14px 18px 16px 10px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
    --label: 52px;
  }

  .head,
  .allday,
  .body {
    display: grid;
    grid-template-columns: var(--label) repeat(var(--days), 1fr);
  }

  .dayhead {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    padding-bottom: 8px;
  }

  .wd {
    font-size: 14px;
    font-weight: 600;
    color: var(--muted);
  }

  .dn {
    display: grid;
    place-items: center;
    width: 32px;
    height: 32px;
    border-radius: 50%;
    font-size: 17px;
    font-weight: 600;
  }

  .dayhead.today .wd {
    color: var(--red);
  }

  .dayhead.today .dn {
    background: var(--red);
    color: #fff;
  }

  .allday {
    grid-template-rows: repeat(var(--rows), 24px);
    row-gap: 3px;
    padding-bottom: 8px;
    border-bottom: 1px solid var(--line);
  }

  .allday .label {
    grid-column: 1;
    grid-row: 1 / -1;
    align-self: center;
    padding-right: 10px;
    font-size: 10.5px;
    line-height: 1.15;
    text-align: right;
    color: var(--faint);
  }

  .bar,
  .more {
    min-width: 0;
    margin: 0 2px;
    padding: 0 9px;
    border-radius: 6px;
    font-size: 12.5px;
    font-weight: 620;
    text-align: left;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .bar {
    background: color-mix(in srgb, var(--cal) 30%, var(--card-solid));
    box-shadow: inset 3px 0 0 var(--cal);
  }

  .more {
    font-weight: 600;
    color: var(--muted);
    background: var(--fill);
  }

  .body {
    position: relative;
    flex: 1;
    min-height: 0;
    margin-top: 8px;
  }

  .hours {
    position: relative;
  }

  .hours span {
    position: absolute;
    right: 10px;
    transform: translateY(-50%);
    font-size: 11.5px;
    color: var(--faint);
  }

  .col {
    position: relative;
    border-left: 1px solid var(--line);
  }

  .line {
    position: absolute;
    left: 0;
    right: 0;
    border-top: 1px solid var(--line);
  }

  .event {
    position: absolute;
    display: flex;
    flex-direction: column;
    gap: 1px;
    padding: 3px 7px;
    border-radius: 7px;
    border-left: 3px solid var(--cal);
    background: color-mix(in srgb, var(--cal) 22%, var(--card-solid));
    overflow: hidden;
    text-align: left;
  }

  .event:active,
  .bar:active {
    opacity: 0.7;
  }

  .event .title {
    font-size: 12.5px;
    font-weight: 620;
    line-height: 1.25;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .event .time {
    font-size: 11px;
    color: var(--muted);
  }

  .now {
    position: absolute;
    left: -4px;
    right: 0;
    height: 2px;
    background: var(--red);
  }

  .now::before {
    content: '';
    position: absolute;
    left: -3px;
    top: -4px;
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--red);
  }
</style>

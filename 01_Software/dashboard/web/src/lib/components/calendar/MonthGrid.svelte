<!-- Month view: weeks from Monday, up to three events per day and "+N więcej". A tap on a day
     opens its day view. -->
<script lang="ts">
  import { clock } from '../../clock.svelte';
  import { addDays, eventsOn, sameDay, showDay, startOfWeek } from '../../calendarView.svelte';
  import { hhmm } from '../../format';
  import type { CalendarEvent } from '../../types';

  let { month, events }: { month: Date; events: CalendarEvent[] } = $props();

  const MAX = 3;
  const WEEKDAYS = ['pon.', 'wt.', 'śr.', 'czw.', 'pt.', 'sob.', 'niedz.'];

  const weeks = $derived.by(() => {
    const first = new Date(month.getFullYear(), month.getMonth(), 1);
    const start = startOfWeek(first);
    const rows: Date[][] = [];
    for (let w = 0; w < 6; w++) {
      const row = Array.from({ length: 7 }, (_, d) => addDays(start, w * 7 + d));
      if (w > 3 && row[0].getMonth() !== month.getMonth()) break; // no all-next-month row
      rows.push(row);
    }
    return rows;
  });
</script>

<div class="month" style:--rows={weeks.length}>
  <div class="weekdays">
    {#each WEEKDAYS as wd (wd)}<span>{wd}</span>{/each}
  </div>
  <div class="cells">
    {#each weeks as week, w (w)}
      {#each week as day (day.toISOString())}
        {@const list = eventsOn(day, events)}
        <button
          class="cell"
          class:other={day.getMonth() !== month.getMonth()}
          class:today={sameDay(day, clock.now)}
          onclick={() => showDay(day)}
        >
          <span class="dn num">{day.getDate()}</span>
          {#each list.slice(0, MAX) as e (e.id)}
            <span class="ev" style:--cal={e.color}>
              <i></i>{#if !e.allDay}<b class="num">{hhmm(e.start)}</b>{/if}{e.title}
            </span>
          {/each}
          {#if list.length > MAX}<span class="more">+{list.length - MAX} więcej</span>{/if}
        </button>
      {/each}
    {/each}
  </div>
</div>

<style>
  .month {
    display: flex;
    flex-direction: column;
    height: 100%;
    padding: 12px 14px 14px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
  }

  .weekdays {
    display: grid;
    grid-template-columns: repeat(7, 1fr);
    padding: 0 6px 8px;
    font-size: 13px;
    font-weight: 600;
    color: var(--muted);
  }

  .cells {
    flex: 1;
    display: grid;
    grid-template-columns: repeat(7, 1fr);
    grid-template-rows: repeat(var(--rows), 1fr);
    min-height: 0;
    border-top: 1px solid var(--line);
  }

  .cell {
    display: flex;
    flex-direction: column;
    align-items: stretch;
    gap: 2px;
    min-width: 0;
    padding: 5px 6px;
    border-bottom: 1px solid var(--line);
    text-align: left;
    overflow: hidden;
  }

  .cell:not(:nth-child(7n)) {
    border-right: 1px solid var(--line);
  }

  .cell:active {
    background: var(--fill);
  }

  .dn {
    display: grid;
    place-items: center;
    width: 26px;
    height: 26px;
    margin-bottom: 1px;
    border-radius: 50%;
    font-size: 14.5px;
    font-weight: 600;
  }

  .cell.today .dn {
    background: var(--red);
    color: #fff;
  }

  .cell.other {
    opacity: 0.4;
  }

  .ev {
    display: flex;
    align-items: center;
    gap: 5px;
    font-size: 11.5px;
    line-height: 1.35;
    white-space: nowrap;
    overflow: hidden;
  }

  .ev i {
    width: 6px;
    height: 6px;
    flex: none;
    border-radius: 50%;
    background: var(--cal);
  }

  .ev b {
    font-weight: 500;
    color: var(--muted);
  }

  .more {
    font-size: 11px;
    color: var(--faint);
  }
</style>

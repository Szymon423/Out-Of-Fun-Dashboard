<!-- Calendars: day, week or month view of the sources that are switched on. The chips under
     the title turn sources on and off (also for the summary widget). -->
<script lang="ts">
  import ChevronLeft from '@lucide/svelte/icons/chevron-left';
  import ChevronRight from '@lucide/svelte/icons/chevron-right';
  import {
    addDays,
    calView,
    setMode,
    startOfWeek,
    step,
    today,
    toggleSource,
    visibleEvents,
    type CalendarMode,
  } from '../calendarView.svelte';
  import MonthGrid from '../components/calendar/MonthGrid.svelte';
  import TimeGrid from '../components/calendar/TimeGrid.svelte';
  import PageFrame from '../components/PageFrame.svelte';
  import Segmented from '../components/Segmented.svelte';
  import { longDate } from '../format';
  import { dash } from '../state.svelte';

  const MODES: { value: CalendarMode; label: string }[] = [
    { value: 'day', label: 'Dzień' },
    { value: 'week', label: 'Tydzień' },
    { value: 'month', label: 'Miesiąc' },
  ];

  const monthFmt = new Intl.DateTimeFormat('pl-PL', { month: 'long', year: 'numeric' });
  const dayMonthFmt = new Intl.DateTimeFormat('pl-PL', { day: 'numeric', month: 'long' });

  const events = $derived(visibleEvents());
  const weekDays = $derived(Array.from({ length: 7 }, (_, i) => addDays(startOfWeek(calView.anchor), i)));

  const period = $derived.by(() => {
    const a = calView.anchor;
    if (calView.mode === 'day') return longDate(a);
    if (calView.mode === 'month') {
      const s = monthFmt.format(a);
      return s.charAt(0).toUpperCase() + s.slice(1);
    }
    const first = weekDays[0];
    const last = weekDays[6];
    return first.getMonth() === last.getMonth()
      ? `${first.getDate()}–${dayMonthFmt.format(last)}`
      : `${dayMonthFmt.format(first)} – ${dayMonthFmt.format(last)}`;
  });
</script>

<PageFrame title="Kalendarz" subtitle={period}>
  {#snippet actions()}
    <div class="nav">
      <button aria-label="Wstecz" onclick={() => step(-1)}><ChevronLeft size={22} /></button>
      <button class="today" onclick={today}>Dziś</button>
      <button aria-label="Dalej" onclick={() => step(1)}><ChevronRight size={22} /></button>
    </div>
    <Segmented options={MODES} value={calView.mode} onchange={setMode} />
  {/snippet}

  <div class="layout">
    <div class="sources" data-interactive>
      {#each dash.calendars ?? [] as c (c.id)}
        <button class="source" class:off={calView.hidden.includes(c.id)} style:--cal={c.color} onclick={() => toggleSource(c.id)}>
          <i></i>{c.name}
        </button>
      {/each}
    </div>
    <div class="view">
      {#if calView.mode === 'month'}
        <MonthGrid month={calView.anchor} {events} />
      {:else if calView.mode === 'week'}
        <TimeGrid onselect={(e) => (calView.selected = e)} days={weekDays} {events} />
      {:else}
        <TimeGrid onselect={(e) => (calView.selected = e)} days={[calView.anchor]} {events} header={false} />
      {/if}
    </div>
  </div>
</PageFrame>

<style>
  .layout {
    display: flex;
    flex-direction: column;
    gap: 14px;
    height: 100%;
  }

  .nav {
    display: flex;
    align-items: center;
    gap: 4px;
    padding: 3px;
    border-radius: 12px;
    background: var(--fill-strong);
  }

  .nav button {
    display: grid;
    place-items: center;
    min-width: 40px;
    height: 40px;
    border-radius: 9px;
    color: var(--text);
  }

  .nav .today {
    padding: 0 14px;
    font-size: 15px;
    font-weight: 600;
    color: var(--red);
  }

  .nav button:active {
    background: var(--card-solid);
  }

  .sources {
    display: flex;
    gap: 10px;
  }

  .source {
    display: flex;
    align-items: center;
    gap: 8px;
    height: 38px;
    padding: 0 16px 0 12px;
    border-radius: 999px;
    font-size: 15px;
    font-weight: 600;
    background: color-mix(in srgb, var(--cal) 20%, transparent);
    box-shadow: 0 0 0 1.5px color-mix(in srgb, var(--cal) 55%, transparent) inset;
    transition: opacity 0.2s ease;
  }

  .source i {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: var(--cal);
  }

  .source.off {
    background: transparent;
    box-shadow: 0 0 0 1.5px var(--line) inset;
    color: var(--faint);
  }

  .source.off i {
    background: transparent;
    box-shadow: 0 0 0 1.5px var(--faint) inset;
  }

  .view {
    flex: 1;
    min-height: 0;
  }
</style>

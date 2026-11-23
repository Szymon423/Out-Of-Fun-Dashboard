<!-- 3D printer at a glance. While it prints: ring with the progress, job name and time left.
     Otherwise, in the manner of the iOS Batteries widget: the four AMS slots as rings in the
     colours of the loaded filament, with the material inside (an empty slot is a thin dashed
     ring), then the printer and its last job or message. A tap opens the printer page.
     (A drawing of the printer did not fit the look; the render of the last model is not
     reachable: the X2D keeps cloud jobs in internal storage, which FTPS does not show.) -->
<script lang="ts">
  import Printer from '@lucide/svelte/icons/printer';
  import { finishesAt, PRINTER_STATE, remaining } from '../../printer';
  import { dash } from '../../state.svelte';
  import ProgressRing from '../ProgressRing.svelte';
  import Widget from '../Widget.svelte';

  const p = $derived(dash.printer);
  const state = $derived(p ? PRINTER_STATE[p.state] : null);
  const busy = $derived(!!p?.job && (p.state === 'printing' || p.state === 'paused'));
  const note = $derived.by(() => {
    if (!p) return '';
    if (p.message) return p.message;
    if (p.state === 'offline') return 'Wyłączona albo poza siecią';
    if (p.state === 'finished' && p.job) return `Gotowe: ${p.job.name}`;
    return 'Czeka na zadanie';
  });
</script>

<Widget to="printer" class="printer">
  {#if p && state}
    <div class="head" style:color={state.color}>
      <Printer size={16} strokeWidth={2.2} />
      <span>{state.label}</span>
    </div>
    {#if busy && p.job}
      <div class="ring">
        <ProgressRing value={p.job.progress} size={118} stroke={11} color={state.color} />
        <div class="pct num">{Math.round(p.job.progress)}<small>%</small></div>
      </div>
      <div class="job">{p.job.name}</div>
      <div class="meta num">{remaining(p.job.remainingMin)} · do {finishesAt(p.job.remainingMin)}</div>
    {:else}
      <div class="spools" class:dim={p.state === 'offline'}>
        {#each p.ams.slice(0, 4) as tray, i (i)}
          <div class="spool" class:none={!tray.color} class:active={tray.active} style:--c={tray.color ?? 'transparent'}>
            <span>{tray.material || '—'}</span>
          </div>
        {/each}
      </div>
      <div class="name">{p.model ? `Bambu Lab ${p.model}` : p.name}</div>
      <div class="meta" class:bad={!!p.message}>{note}</div>
    {/if}
  {:else}
    <div class="idle">Łączę z drukarką…</div>
  {/if}
</Widget>

<style>
  .head {
    display: flex;
    align-items: center;
    gap: 7px;
    font-size: 14px;
    font-weight: 650;
  }

  .ring {
    position: relative;
    display: grid;
    place-items: center;
    margin: 18px auto 14px;
    width: 118px;
  }

  .pct {
    position: absolute;
    font-size: 32px;
    font-weight: 600;
    letter-spacing: -0.02em;
  }

  .pct small {
    font-size: 16px;
    font-weight: 500;
    color: var(--muted);
  }

  .job {
    font-size: 16.5px;
    font-weight: 600;
    text-align: center;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .meta {
    margin-top: 3px;
    font-size: 14px;
    color: var(--muted);
    text-align: center;
  }

  .idle {
    margin-top: 20px;
    color: var(--faint);
    font-size: 15px;
  }

  .spools {
    display: grid;
    grid-template-columns: repeat(2, 62px);
    justify-content: center;
    gap: 12px 26px;
    margin: 16px 0 14px;
    transition: opacity 0.4s ease;
  }

  .spools.dim {
    opacity: 0.4;
  }

  /* A ring in the filament's colour; the hairline keeps white and black spools visible on
     either theme. Zero-blur shadows only (docs/plan.md). */
  .spool {
    display: grid;
    place-items: center;
    width: 62px;
    height: 62px;
    border-radius: 50%;
    border: 7px solid var(--c);
    box-shadow:
      0 0 0 1px var(--fill-strong),
      inset 0 0 0 1px var(--fill-strong);
  }

  .spool span {
    font-size: 11.5px;
    font-weight: 700;
    letter-spacing: 0.02em;
    color: var(--muted);
  }

  .spool.active span {
    color: var(--text);
  }

  .spool.active {
    box-shadow:
      0 0 0 1px var(--fill-strong),
      0 0 0 4px var(--card-solid),
      0 0 0 5px var(--c);
  }

  .spool.none {
    border: 1.5px dashed var(--fill-strong);
    box-shadow: none;
  }

  .name {
    font-size: 16px;
    font-weight: 650;
    text-align: center;
  }

  .meta {
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .meta.bad {
    color: var(--bad);
  }
</style>

<!-- 3D printer at a glance: ring with the progress, job name and time left. A tap opens the
     printer page. -->
<script lang="ts">
  import Printer from '@lucide/svelte/icons/printer';
  import { finishesAt, PRINTER_STATE, remaining } from '../../printer';
  import { dash } from '../../state.svelte';
  import ProgressRing from '../ProgressRing.svelte';
  import Widget from '../Widget.svelte';

  const p = $derived(dash.printer);
  const state = $derived(p ? PRINTER_STATE[p.state] : null);
</script>

<Widget to="printer" class="printer">
  {#if p && state}
    <div class="head" style:color={state.color}>
      <Printer size={16} strokeWidth={2.2} />
      <span>{state.label}</span>
    </div>
    {#if p.job && (p.state === 'printing' || p.state === 'paused')}
      <div class="ring">
        <ProgressRing value={p.job.progress} size={118} stroke={11} color={state.color} />
        <div class="pct num">{Math.round(p.job.progress)}<small>%</small></div>
      </div>
      <div class="job">{p.job.name}</div>
      <div class="meta num">{remaining(p.job.remainingMin)} · do {finishesAt(p.job.remainingMin)}</div>
    {:else}
      <div class="idle">{p.message ?? `${p.model} czeka na zadanie`}</div>
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
</style>

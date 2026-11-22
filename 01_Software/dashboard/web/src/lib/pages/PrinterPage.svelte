<!-- Bambu Lab printer: job progress, temperatures and the AMS filament slots. -->
<script lang="ts">
  import Box from '@lucide/svelte/icons/box';
  import Flame from '@lucide/svelte/icons/flame';
  import Heater from '@lucide/svelte/icons/heater';
  import Layers from '@lucide/svelte/icons/layers';
  import ThermometerSun from '@lucide/svelte/icons/thermometer-sun';
  import Timer from '@lucide/svelte/icons/timer';
  import PageFrame from '../components/PageFrame.svelte';
  import ProgressRing from '../components/ProgressRing.svelte';
  import { finishesAt, PRINTER_STATE, remaining } from '../printer';
  import { dash } from '../state.svelte';

  const p = $derived(dash.printer);
  const state = $derived(p ? PRINTER_STATE[p.state] : null);
</script>

<PageFrame title="Drukarka" subtitle={p ? `Bambu Lab ${p.model}` : ''}>
  {#if p && state}
    <div class="layout">
      <section class="job card">
        <div class="ring">
          <ProgressRing value={p.job?.progress ?? 0} size={250} stroke={20} color={state.color} />
          <div class="center">
            <div class="pct num">{Math.round(p.job?.progress ?? 0)}<small>%</small></div>
            <div class="state" style:color={state.color}>{state.label}</div>
          </div>
        </div>
        {#if p.job}
          <div class="info">
            <div class="name">{p.job.name}</div>
            <div class="facts num">
              <span><Timer size={18} /> {remaining(p.job.remainingMin)} · koniec o {finishesAt(p.job.remainingMin)}</span>
              <span><Layers size={18} /> warstwa {p.job.layer} z {p.job.totalLayers}</span>
            </div>
          </div>
        {/if}
      </section>

      <div class="side">
        <section class="card temps num">
          <div class="temp"><Flame size={20} /><span>Dysza</span><b>{Math.round(p.nozzle.temp)}°</b><small>/ {p.nozzle.target}°</small></div>
          <div class="temp"><Heater size={20} /><span>Stół</span><b>{Math.round(p.bed.temp)}°</b><small>/ {p.bed.target}°</small></div>
          {#if p.chamber !== null}
            <div class="temp"><ThermometerSun size={20} /><span>Komora</span><b>{Math.round(p.chamber)}°</b></div>
          {/if}
        </section>

        <section class="card ams">
          <div class="title"><Box size={16} /> AMS</div>
          <div class="trays">
            {#each p.ams as tray, i (i)}
              <div class="tray" class:active={tray.active}>
                <span class="spool" style:background={tray.color}></span>
                <span class="mat">{tray.material}</span>
                <span class="left num">{tray.remaining === null ? '—' : `${tray.remaining}%`}</span>
              </div>
            {/each}
          </div>
        </section>
      </div>
    </div>
  {:else}
    <p class="empty">Łączę z drukarką…</p>
  {/if}
</PageFrame>

<style>
  .layout {
    display: grid;
    grid-template-columns: 1fr 420px;
    gap: var(--gap);
    height: 100%;
  }

  .card {
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
    padding: 22px 24px;
  }

  .job {
    display: flex;
    align-items: center;
    gap: 34px;
    padding: 30px 36px;
  }

  .ring {
    position: relative;
    display: grid;
    place-items: center;
    flex: none;
  }

  .center {
    position: absolute;
    text-align: center;
  }

  .pct {
    font-size: 66px;
    font-weight: 300;
    letter-spacing: -0.03em;
    line-height: 1;
  }

  .pct small {
    font-size: 26px;
    color: var(--muted);
  }

  .state {
    margin-top: 6px;
    font-size: 17px;
    font-weight: 650;
  }

  .info {
    min-width: 0;
  }

  .name {
    font-size: 28px;
    font-weight: 680;
    letter-spacing: -0.01em;
  }

  .facts {
    display: flex;
    flex-direction: column;
    gap: 10px;
    margin-top: 16px;
    font-size: 17px;
    color: var(--muted);
  }

  .facts span {
    display: inline-flex;
    align-items: center;
    gap: 10px;
  }

  .side {
    display: flex;
    flex-direction: column;
    gap: var(--gap);
  }

  .temps {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .temp {
    display: flex;
    align-items: center;
    gap: 12px;
    font-size: 18px;
    color: var(--muted);
  }

  .temp span {
    flex: 1;
    color: var(--text);
  }

  .temp b {
    font-size: 24px;
    font-weight: 600;
    color: var(--text);
  }

  .temp small {
    width: 52px;
    font-size: 15px;
  }

  .ams {
    flex: 1;
  }

  .ams .title {
    display: flex;
    align-items: center;
    gap: 8px;
    font-size: 13px;
    font-weight: 650;
    letter-spacing: 0.1em;
    color: var(--faint);
  }

  .trays {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 10px;
    margin-top: 16px;
  }

  .tray {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
    padding: 12px 4px;
    border-radius: 16px;
  }

  .tray.active {
    background: var(--fill);
  }

  .spool {
    width: 48px;
    height: 48px;
    border-radius: 50%;
    box-shadow:
      0 0 0 4px var(--fill-strong),
      0 4px 10px rgba(0, 0, 0, 0.25) inset;
  }

  .mat {
    margin-top: 4px;
    font-size: 14px;
    font-weight: 600;
  }

  .left {
    font-size: 13px;
    color: var(--muted);
  }

  .empty {
    color: var(--faint);
  }
</style>

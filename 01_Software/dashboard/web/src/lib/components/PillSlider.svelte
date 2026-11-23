<!-- A tall Control Center pill: filled from the bottom, dragged up and down anywhere on it.
     The fill is a scaleY transform, so dragging repaints nothing. `fill` is any CSS
     background (a colour or a gradient). The icon sits at the bottom, the label and the value
     under the pill. Changes go out at most every 150 ms while dragging and once more on
     release; data-noswipe keeps the pager from taking the drag as a page swipe. -->
<script lang="ts">
  import type { Component } from 'svelte';

  let {
    value,
    onchange,
    min = 0,
    max = 100,
    fill = 'var(--text)',
    icon: Icon,
    label,
    dim = false,
  }: {
    value: number;
    onchange: (v: number) => void;
    min?: number;
    max?: number;
    fill?: string;
    icon?: Component;
    label: string;
    dim?: boolean;
  } = $props();

  let el: HTMLElement | undefined = $state();
  let local = $state<number | null>(null); // value while dragging
  let lastSent = 0;
  const shown = $derived(local ?? value);
  const ratio = $derived((shown - min) / (max - min));

  function fromEvent(e: PointerEvent): number {
    const r = el!.getBoundingClientRect();
    const y = Math.max(0, Math.min(1, (r.bottom - e.clientY) / r.height));
    return Math.round(min + y * (max - min));
  }

  function down(e: PointerEvent): void {
    el!.setPointerCapture(e.pointerId);
    local = fromEvent(e);
    lastSent = 0;
    move(e);
  }

  function move(e: PointerEvent): void {
    if (local === null) return;
    local = fromEvent(e);
    if (e.timeStamp - lastSent > 150) {
      lastSent = e.timeStamp;
      onchange(local);
    }
  }

  function up(): void {
    if (local === null) return;
    onchange(local);
    local = null;
  }
</script>

<div class="wrap">
  <div
    class="pill"
    class:dim
    bind:this={el}
    style:--fill={fill}
    role="slider"
    aria-label={label}
    aria-orientation="vertical"
    aria-valuenow={shown}
    aria-valuemin={min}
    aria-valuemax={max}
    tabindex="-1"
    data-noswipe
    onpointerdown={down}
    onpointermove={move}
    onpointerup={up}
    onpointercancel={up}
  >
    <i class="fill" style:transform="scaleY({ratio})"></i>
    {#if Icon}<span class="icon" class:covered={ratio > 0.12}><Icon size={26} strokeWidth={2} /></span>{/if}
  </div>
  <div class="caption">
    <span>{label}</span>
    <b class="num">{shown}%</b>
  </div>
</div>

<style>
  .wrap {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    height: 100%;
  }

  .pill {
    position: relative;
    flex: 1;
    width: 112px;
    border-radius: 34px;
    background: var(--fill-strong);
    overflow: hidden;
    touch-action: none;
    cursor: pointer;
  }

  .fill {
    position: absolute;
    inset: 0;
    background: var(--fill);
    transform-origin: 50% 100%;
    will-change: transform;
    transition: opacity 0.3s ease;
  }

  .pill.dim .fill {
    opacity: 0.35;
  }

  .icon {
    position: absolute;
    left: 50%;
    bottom: 20px;
    transform: translateX(-50%);
    display: grid;
    color: var(--muted);
    pointer-events: none;
  }

  .icon.covered {
    color: rgba(255, 255, 255, 0.92);
  }

  .caption {
    display: flex;
    flex-direction: column;
    align-items: center;
    line-height: 1.2;
  }

  .caption span {
    font-size: 14px;
    color: var(--muted);
  }

  .caption b {
    font-size: 20px;
    font-weight: 650;
  }
</style>

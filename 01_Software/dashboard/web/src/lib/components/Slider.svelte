<!-- Control Center style slider: a pill filled from the left, dragged anywhere along it. The
     fill is a scaleX transform, so dragging repaints nothing. `track` replaces the fill with a
     background (e.g. a hue gradient) and shows a knob instead. Changes go out at most every
     150 ms while dragging and once more on release. data-noswipe keeps the pager from
     taking the drag as a page swipe. -->
<script lang="ts">
  import type { Component } from 'svelte';

  let {
    value,
    onchange,
    min = 0,
    max = 100,
    icon: Icon,
    track = '',
    label = '',
    height = 44,
  }: {
    value: number;
    onchange: (v: number) => void;
    min?: number;
    max?: number;
    icon?: Component;
    track?: string;
    label?: string;
    height?: number;
  } = $props();

  let el: HTMLElement | undefined = $state();
  let local = $state<number | null>(null); // value while dragging
  let lastSent = 0;
  const shown = $derived(local ?? value);
  const ratio = $derived((shown - min) / (max - min));

  function fromEvent(e: PointerEvent): number {
    const r = el!.getBoundingClientRect();
    const x = Math.max(0, Math.min(1, (e.clientX - r.left) / r.width));
    return Math.round(min + x * (max - min));
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

<div
  class="slider"
  class:custom={!!track}
  bind:this={el}
  style:height="{height}px"
  style:--track={track}
  role="slider"
  aria-label={label}
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
  {#if track}
    <i class="knob" style:left="calc({ratio * 100}% - {ratio * height}px)" style:width="{height}px"></i>
  {:else}
    <i class="fill" style:transform="scaleX({ratio})"></i>
  {/if}
  {#if Icon}<span class="icon" class:covered={!track && ratio > 0.1}><Icon size={20} strokeWidth={2} /></span>{/if}
</div>

<style>
  .slider {
    position: relative;
    flex: 1;
    min-width: 120px;
    border-radius: 999px;
    background: var(--fill-strong);
    overflow: hidden;
    touch-action: none;
    cursor: pointer;
  }

  .slider.custom {
    background: var(--track);
    overflow: visible;
  }

  .fill {
    position: absolute;
    inset: 0;
    background: var(--text);
    transform-origin: 0 50%;
    will-change: transform;
  }

  .knob {
    position: absolute;
    top: 0;
    bottom: 0;
    border-radius: 50%;
    background: transparent;
    box-shadow:
      0 0 0 4px #fff,
      0 2px 10px rgba(0, 0, 0, 0.35);
  }

  .icon {
    position: absolute;
    left: 14px;
    top: 50%;
    transform: translateY(-50%);
    display: grid;
    color: var(--muted);
    pointer-events: none;
    transition: color 0.15s ease;
  }

  .icon.covered {
    color: var(--card-solid);
  }
</style>

<!-- Volume, quiet until touched: a thin line with a dot at rest, the thick Control Center pill
     only while the finger is on it (and a moment after). The two looks cross-fade with
     opacity; the fill and the dot move by transform, so nothing is repainted while dragging.
     The whole 44 px is the touch target in both looks. Changes go out at most every 150 ms
     while dragging and once more on release. -->
<script lang="ts">
  import type { Component } from 'svelte';

  let { value, onchange, icon: Icon, label = '' }: {
    value: number;
    onchange: (v: number) => void;
    icon?: Component;
    label?: string;
  } = $props();

  let el: HTMLElement | undefined = $state();
  let local = $state<number | null>(null); // value while dragging
  let lastSent = 0;
  const shown = $derived(local ?? value);
  const ratio = $derived(Math.max(0, Math.min(1, shown / 100)));

  function fromEvent(e: PointerEvent): number {
    const r = el!.getBoundingClientRect();
    return Math.round(Math.max(0, Math.min(1, (e.clientX - r.left) / r.width)) * 100);
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
  class="volume"
  class:active={local !== null}
  bind:this={el}
  role="slider"
  aria-label={label}
  aria-valuenow={shown}
  aria-valuemin={0}
  aria-valuemax={100}
  tabindex="-1"
  data-noswipe
  onpointerdown={down}
  onpointermove={move}
  onpointerup={up}
  onpointercancel={up}
>
  <div class="thin">
    {#if Icon}<span class="icon"><Icon size={18} strokeWidth={2} /></span>{/if}
    <div class="line">
      <i class="fill" style:transform="scaleX({ratio})"></i>
      <i class="rail" style:transform="translateX({ratio * 100}%)"><b></b></i>
    </div>
  </div>
  <div class="thick">
    <i class="fill" style:transform="scaleX({ratio})"></i>
    {#if Icon}<span class="icon" class:covered={ratio > 0.1}><Icon size={20} strokeWidth={2} /></span>{/if}
  </div>
</div>

<style>
  .volume {
    position: relative;
    flex: 1;
    min-width: 120px;
    height: 44px;
    touch-action: none;
    cursor: pointer;
  }

  .thin,
  .thick {
    position: absolute;
    inset: 0;
    transition: opacity 0.25s ease 0.35s; /* back to thin a moment after release */
  }

  .active .thin,
  .active .thick {
    transition-delay: 0s;
  }

  .thin {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .active .thin {
    opacity: 0;
  }

  .thin .icon {
    display: grid;
    color: var(--muted);
  }

  .line {
    position: relative;
    flex: 1;
    height: 4px;
    border-radius: 2px;
    background: var(--fill-strong);
  }

  .line .fill {
    position: absolute;
    inset: 0;
    border-radius: 2px;
    background: var(--muted);
    transform-origin: 0 50%;
    will-change: transform;
  }

  .rail {
    position: absolute;
    inset: 0;
    will-change: transform;
  }

  .rail b {
    position: absolute;
    left: -8px;
    top: -6px;
    width: 16px;
    height: 16px;
    border-radius: 50%;
    background: var(--text);
    opacity: 0.85;
  }

  .thick {
    border-radius: 999px;
    background: var(--fill-strong);
    overflow: hidden;
    opacity: 0;
  }

  .active .thick {
    opacity: 1;
  }

  .thick .fill {
    position: absolute;
    inset: 0;
    background: var(--text);
    transform-origin: 0 50%;
    will-change: transform;
  }

  .thick .icon {
    position: absolute;
    left: 14px;
    top: 50%;
    transform: translateY(-50%);
    display: grid;
    color: var(--muted);
    pointer-events: none;
  }

  .thick .icon.covered {
    color: var(--card-solid);
  }
</style>

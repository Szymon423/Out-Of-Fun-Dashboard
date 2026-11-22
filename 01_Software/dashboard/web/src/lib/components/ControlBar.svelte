<!--
  Control bar pulled down from the top edge: theme, screen brightness and the screensaver.
  It follows the finger while pulled (nav.pull, set by the pager), and closes with a tap
  beside it, a swipe up on it, or by itself after 8 s without a touch. Moves by transform.
-->
<script lang="ts">
  import type { Component } from 'svelte';
  import Moon from '@lucide/svelte/icons/moon';
  import MoonStar from '@lucide/svelte/icons/moon-star';
  import Sun from '@lucide/svelte/icons/sun';
  import SunDim from '@lucide/svelte/icons/sun-dim';
  import SunMoon from '@lucide/svelte/icons/sun-moon';
  import { nav, openSaver } from '../nav.svelte';
  import { dash, setBrightness } from '../state.svelte';
  import { activeMode, setThemeMode, type ThemeMode } from '../theme.svelte';
  import Segmented from './Segmented.svelte';
  import Slider from './Slider.svelte';

  const HIDDEN = -150; // px above the screen when closed
  const AUTO_CLOSE_MS = 8000;

  const MODES: { value: ThemeMode; label: string; icon: Component }[] = [
    { value: 'auto', label: 'Auto', icon: SunMoon },
    { value: 'light', label: 'Jasny', icon: Sun },
    { value: 'dark', label: 'Ciemny', icon: Moon },
  ];

  const y = $derived.by(() => {
    if (nav.pull !== null) return Math.min(0, HIDDEN + nav.pull); // following the finger
    return nav.controls ? 0 : HIDDEN;
  });

  // Close by itself after a while. The wait starts again when the bar opens and with every
  // touch on it.
  let lastTouch = $state(0);
  $effect(() => {
    if (!nav.controls) return;
    void lastTouch; // a touch restarts the wait
    const timer = setTimeout(() => (nav.controls = false), AUTO_CLOSE_MS);
    return () => clearTimeout(timer);
  });

  // Swipe up on the bar closes it.
  let startY: number | null = null;
  function down(e: PointerEvent): void {
    lastTouch = Date.now();
    startY = e.target instanceof Element && e.target.closest('[data-noswipe]') ? null : e.clientY;
  }
  function up(e: PointerEvent): void {
    if (startY !== null && e.clientY - startY < -30) nav.controls = false;
    startY = null;
  }
</script>

{#if nav.controls}
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div class="outside" onpointerdown={() => (nav.controls = false)}></div>
{/if}

<!-- svelte-ignore a11y_no_static_element_interactions -->
<div
  class="bar"
  class:following={nav.pull !== null}
  style:transform="translate3d(-50%, {y}px, 0)"
  onpointerdown={down}
  onpointerup={up}
>
  <Segmented options={MODES} value={activeMode()} onchange={setThemeMode} />
  <div class="brightness">
    <Slider value={dash.display?.brightness ?? 80} min={5} icon={SunDim} label="Jasność ekranu" onchange={setBrightness} />
  </div>
  <button class="saver" onclick={openSaver}>
    <MoonStar size={19} strokeWidth={2} />
    Wygaszacz
  </button>
  <i class="grabber"></i>
</div>

<style>
  .outside {
    position: absolute;
    inset: 0;
    z-index: 5;
  }

  .bar {
    position: absolute;
    left: 50%;
    top: 14px;
    z-index: 6;
    display: flex;
    align-items: center;
    gap: 18px;
    width: 1000px;
    padding: 20px 22px 26px;
    border-radius: 30px;
    background: var(--card-solid);
    box-shadow:
      0 20px 60px rgba(0, 0, 0, 0.35),
      0 0 0 1px var(--line);
    will-change: transform;
    transition: transform 0.38s var(--ease);
  }

  .bar.following {
    transition: none;
  }

  .brightness {
    display: flex;
    flex: 1;
  }

  .saver {
    display: flex;
    align-items: center;
    gap: 9px;
    height: 46px;
    padding: 0 18px;
    border-radius: 14px;
    background: var(--fill-strong);
    font-size: 15px;
    font-weight: 600;
  }

  .saver:active {
    transform: scale(0.96);
  }

  .grabber {
    position: absolute;
    left: 50%;
    bottom: 8px;
    width: 44px;
    height: 5px;
    margin-left: -22px;
    border-radius: 3px;
    background: var(--fill-strong);
  }
</style>

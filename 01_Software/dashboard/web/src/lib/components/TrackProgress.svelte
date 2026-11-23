<!-- Track progress: a single scaleX animation from the current position to the end of the
     track, run by the compositor, plus the elapsed time text updated once a second.
     `middle` goes between the two times, e.g. the play controls. -->
<script lang="ts">
  import type { Snippet } from 'svelte';
  import { duration } from '../format';
  import type { NowPlaying } from '../types';

  let { music, times = true, middle }: { music: NowPlaying; times?: boolean; middle?: Snippet } = $props();

  let fill: HTMLElement | undefined = $state();
  let elapsedMs = $state(0);

  function position(): number {
    return Math.min(music.durationMs, music.progressMs + (music.isPlaying ? Date.now() - music.readAt : 0));
  }

  $effect(() => {
    if (!fill) return;
    const pos = position();
    const ratio = music.durationMs ? pos / music.durationMs : 0;
    for (const a of fill.getAnimations()) a.cancel();
    fill.style.transform = `scaleX(${ratio})`;
    if (music.isPlaying && music.durationMs > pos) {
      fill.animate([{ transform: `scaleX(${ratio})` }, { transform: 'scaleX(1)' }], {
        duration: music.durationMs - pos,
        easing: 'linear',
        fill: 'forwards',
      });
    }
  });

  $effect(() => {
    elapsedMs = position();
    if (!times || !music.isPlaying) return;
    const timer = setInterval(() => (elapsedMs = position()), 1000);
    return () => clearInterval(timer);
  });
</script>

<div class="progress">
  <div class="track"><i bind:this={fill}></i></div>
  {#if times}
    <div class="times num">
      <span>{duration(elapsedMs)}</span>
      {@render middle?.()}
      <span>−{duration(music.durationMs - elapsedMs)}</span>
    </div>
  {/if}
</div>

<style>
  .track {
    height: 4px;
    border-radius: 2px;
    background: var(--fill-strong);
    overflow: hidden;
  }

  .track i {
    display: block;
    height: 100%;
    background: var(--text);
    transform-origin: 0 50%;
    transform: scaleX(0);
    will-change: transform;
  }

  .times {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-top: 6px;
    font-size: 12.5px;
    color: var(--faint);
  }
</style>

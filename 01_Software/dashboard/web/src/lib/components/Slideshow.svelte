<!-- Photo slideshow with crossfade and a slow zoom (Ken Burns), used by the photos page and
     the screensaver. Only two images exist at a time; both effects animate opacity and
     transform, so the compositor does all the work between photo changes. -->
<script lang="ts">
  import { fade } from 'svelte/transition';
  import type { Photo } from '../types';

  let {
    photos,
    intervalMs = 15000,
    running = true,
    current = $bindable(0),
  }: { photos: Photo[]; intervalMs?: number; running?: boolean; current?: number } = $props();

  const photo = $derived(photos.length ? photos[current % photos.length] : null);

  $effect(() => {
    if (!running || photos.length < 2) return;
    const timer = setInterval(() => (current = (current + 1) % photos.length), intervalMs);
    return () => clearInterval(timer);
  });
</script>

<div class="show" style:--zoom-ms="{intervalMs + 2500}ms">
  {#if photo}
    {#key photo.id}
      <img
        src={photo.url}
        alt=""
        class:even={current % 2 === 0}
        in:fade={{ duration: 1600 }}
        out:fade={{ duration: 1600 }}
      />
    {/key}
  {/if}
</div>

<style>
  .show {
    position: absolute;
    inset: 0;
    overflow: hidden;
    background: #000;
  }

  img {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
    will-change: transform, opacity;
    transform-origin: 30% 40%;
    animation: zoom var(--zoom-ms) ease-out forwards;
  }

  img.even {
    transform-origin: 70% 60%;
  }

  @keyframes zoom {
    from {
      transform: scale(1);
    }
    to {
      transform: scale(1.08);
    }
  }
</style>

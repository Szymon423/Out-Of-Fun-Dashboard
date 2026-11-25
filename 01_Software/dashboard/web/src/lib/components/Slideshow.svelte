<!-- Photo slideshow with crossfade and a slow zoom (Ken Burns), used by the photos page and
     the screensaver. It shows `current`; the shared photo clock (photoClock.svelte.ts) moves
     it. Only two images exist at a time; both effects animate opacity and transform, so the
     compositor does all the work between photo changes. The neighbours of the current photo
     are decoded in advance, so a tap either way starts its crossfade with a ready picture. -->
<script lang="ts">
  import { fade } from 'svelte/transition';
  import { preload } from '../photoClock.svelte';
  import type { Photo } from '../types';

  let { photos, current, zoomMs }: { photos: Photo[]; current: number; zoomMs: number } = $props();

  const photo = $derived(photos.length ? photos[current % photos.length] : null);

  // The neighbours of the current photo, ready for a tap either way.
  $effect(() => {
    const n = photos.length;
    if (n < 2) return;
    void preload(photos[(current + 1) % n].url);
    void preload(photos[(current - 1 + n) % n].url);
  });
</script>

<div class="show" style:--zoom-ms="{zoomMs}ms">
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

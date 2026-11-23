<!-- Photo slideshow with crossfade and a slow zoom (Ken Burns), used by the photos page and
     the screensaver. Only two images exist at a time; both effects animate opacity and
     transform, so the compositor does all the work between photo changes. The next and the
     previous photo are decoded in advance, so a change (by itself or by a tap) starts its
     crossfade with a ready picture. Changing `current` from outside restarts the wait. -->
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

  /** Fetches and decodes a photo off the main thread; later <img> uses reuse the result. */
  function preload(url: string): Promise<void> {
    const img = new Image();
    img.src = url;
    return img.decode().catch(() => {}); // a broken photo still gets its turn
  }

  // The neighbours of the current photo, ready for a tap either way.
  $effect(() => {
    const n = photos.length;
    if (n < 2) return;
    void preload(photos[(current + 1) % n].url);
    void preload(photos[(current - 1 + n) % n].url);
  });

  $effect(() => {
    if (!running || photos.length < 2) return;
    const from = current; // read here, so any change of current restarts the wait
    let cancelled = false;
    const timer = setTimeout(async () => {
      const next = (from + 1) % photos.length;
      await preload(photos[next].url);
      if (!cancelled) current = next;
    }, intervalMs);
    return () => {
      cancelled = true;
      clearTimeout(timer);
    };
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

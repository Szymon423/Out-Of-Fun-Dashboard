<!-- Immich photos, full screen with crossfade and slow zoom, a caption and the album name.
     Runs only while this page is the current one. -->
<script lang="ts">
  import Images from '@lucide/svelte/icons/images';
  import Slideshow from '../components/Slideshow.svelte';
  import { nav, PAGES } from '../nav.svelte';
  import { dash } from '../state.svelte';

  const dateFmt = new Intl.DateTimeFormat('pl-PL', { day: 'numeric', month: 'long', year: 'numeric' });
  const photos = $derived(dash.photos?.items ?? []);
  const visible = $derived(PAGES[nav.index]?.id === 'photos' && !nav.overview && !nav.saver);
  let current = $state(0);
  const photo = $derived(photos.length ? photos[current % photos.length] : null);
</script>

<div class="page">
  {#if photos.length}
    <Slideshow {photos} intervalMs={12000} running={visible} bind:current />
    <div class="scrim"></div>
    <header>
      <h1>Zdjęcia</h1>
      <span>{dash.photos?.album}</span>
    </header>
    {#if photo}
      <div class="caption">
        <div class="place">{photo.place ?? ''}</div>
        <div class="date">{dateFmt.format(new Date(photo.takenAt))}</div>
      </div>
    {/if}
  {:else}
    <div class="empty"><Images size={48} strokeWidth={1.4} /><span>Łączę z Immichem…</span></div>
  {/if}
</div>

<style>
  .page {
    position: absolute;
    inset: 0;
    overflow: hidden;
  }

  .scrim {
    position: absolute;
    inset: 0;
    background:
      linear-gradient(to bottom, rgba(0, 0, 0, 0.45), transparent 22%),
      linear-gradient(to top, rgba(0, 0, 0, 0.55), transparent 30%);
  }

  header {
    position: absolute;
    left: var(--pad);
    top: 26px;
    display: flex;
    align-items: baseline;
    gap: 14px;
    color: #fff;
  }

  h1 {
    margin: 0;
    font-size: 34px;
    font-weight: 720;
    letter-spacing: -0.02em;
  }

  header span {
    font-size: 17px;
    opacity: 0.8;
  }

  .caption {
    position: absolute;
    left: var(--pad);
    bottom: 28px;
    color: #fff;
  }

  .place {
    font-size: 30px;
    font-weight: 680;
  }

  .date {
    margin-top: 2px;
    font-size: 17px;
    opacity: 0.85;
  }

  .empty {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 12px;
    height: 100%;
    color: var(--faint);
  }
</style>

<!-- Immich memory ("Rok temu"), full bleed like the Photos widget. Static on purpose: the
     moving slideshow lives on the photos page and in the screensaver. -->
<script lang="ts">
  import Images from '@lucide/svelte/icons/images';
  import { dash } from '../../state.svelte';
  import Widget from '../Widget.svelte';

  const year = new Intl.DateTimeFormat('pl-PL', { day: 'numeric', month: 'long', year: 'numeric' });
  const photo = $derived(dash.photos?.memory?.photo ?? dash.photos?.items[0] ?? null);
</script>

<Widget to="photos" flush class="photo">
  {#if photo}
    <img src={photo.url} alt="" />
    <div class="scrim"></div>
    <div class="caption">
      <div class="title">{dash.photos?.memory?.title ?? dash.photos?.album}</div>
      <div class="meta">{photo.place ?? ''}{photo.place ? ' · ' : ''}{year.format(new Date(photo.takenAt))}</div>
    </div>
  {:else}
    <div class="empty"><Images size={34} strokeWidth={1.5} /><span>Zdjęcia z Immicha</span></div>
  {/if}
</Widget>

<style>
  img {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .scrim {
    position: absolute;
    inset: 0;
    background: linear-gradient(to top, rgba(0, 0, 0, 0.55), transparent 50%);
  }

  .caption {
    position: absolute;
    left: 20px;
    right: 20px;
    bottom: 18px;
    color: #fff;
  }

  .title {
    font-size: 20px;
    font-weight: 650;
  }

  .meta {
    margin-top: 2px;
    font-size: 14px;
    opacity: 0.85;
  }

  .empty {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 10px;
    height: 100%;
    color: var(--faint);
    font-size: 15px;
  }
</style>

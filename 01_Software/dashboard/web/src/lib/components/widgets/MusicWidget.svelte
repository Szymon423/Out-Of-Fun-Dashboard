<!-- Now playing, compact: cover, title, progress and play/pause. A tap elsewhere opens the
     music page. -->
<script lang="ts">
  import Disc3 from '@lucide/svelte/icons/disc-3';
  import Pause from '@lucide/svelte/icons/pause';
  import Play from '@lucide/svelte/icons/play';
  import SkipForward from '@lucide/svelte/icons/skip-forward';
  import Speaker from '@lucide/svelte/icons/speaker';
  import { dash, music } from '../../state.svelte';
  import TrackProgress from '../TrackProgress.svelte';
  import Widget from '../Widget.svelte';

  const m = $derived(dash.music);
</script>

<Widget to="music" class="music">
  {#if m}
    <div class="top">
      <span class="source">Spotify</span>
      <span class="device"><Speaker size={14} /> {m.device}</span>
    </div>
    <div class="row">
      <div class="art">
        {#if m.artUrl}<img src={m.artUrl} alt="" />{:else}<div class="ph"><Disc3 size={32} strokeWidth={1.4} /></div>{/if}
      </div>
      <div class="info">
        <div class="title">{m.title}</div>
        <div class="artist">{m.artist}</div>
        <div class="album">{m.album}</div>
      </div>
      <div class="controls">
        <button class="main" aria-label={m.isPlaying ? 'Pauza' : 'Graj'} onclick={() => music(m.isPlaying ? 'pause' : 'play')}>
          {#if m.isPlaying}<Pause size={24} fill="currentColor" strokeWidth={0} />{:else}<Play size={24} fill="currentColor" strokeWidth={0} />{/if}
        </button>
        <button aria-label="Następny" onclick={() => music('next')}><SkipForward size={22} fill="currentColor" /></button>
      </div>
    </div>
    <div class="bar"><TrackProgress music={m} /></div>
  {:else}
    <div class="empty">Nic teraz nie gra</div>
  {/if}
</Widget>

<style>
  .top {
    display: flex;
    justify-content: space-between;
    margin-bottom: 16px;
    font-size: 13.5px;
    color: var(--muted);
  }

  .source {
    font-weight: 650;
    color: var(--good);
  }

  .device {
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }

  .album {
    margin-top: 1px;
    font-size: 13.5px;
    color: var(--faint);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .row {
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .art {
    width: 76px;
    height: 76px;
    flex: none;
    border-radius: 14px;
    overflow: hidden;
    box-shadow: 0 0 0 1px var(--line);
  }

  .art img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .ph {
    display: grid;
    place-items: center;
    width: 100%;
    height: 100%;
    color: rgba(255, 255, 255, 0.75);
    background: linear-gradient(135deg, #5e5ce6, #ff375f 55%, #ff9f0a);
  }

  .info {
    flex: 1;
    min-width: 0;
  }

  .title {
    font-size: 19px;
    font-weight: 640;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .artist {
    margin-top: 2px;
    font-size: 15px;
    color: var(--muted);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .controls {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  .controls button {
    display: grid;
    place-items: center;
    width: 48px;
    height: 48px;
    border-radius: 50%;
    color: var(--text);
  }

  .controls .main {
    width: 54px;
    height: 54px;
    background: var(--accent);
    color: var(--accent-ink);
  }

  .controls button:active {
    transform: scale(0.92);
  }

  .bar {
    margin-top: 16px;
  }

  .empty {
    color: var(--faint);
    font-size: 16px;
  }
</style>

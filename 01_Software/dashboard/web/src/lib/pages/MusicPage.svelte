<!-- Music: Apple Music style now playing, big cover on the left, controls on the right. -->
<script lang="ts">
  import Disc3 from '@lucide/svelte/icons/disc-3';
  import Pause from '@lucide/svelte/icons/pause';
  import Play from '@lucide/svelte/icons/play';
  import SkipBack from '@lucide/svelte/icons/skip-back';
  import SkipForward from '@lucide/svelte/icons/skip-forward';
  import Speaker from '@lucide/svelte/icons/speaker';
  import PageFrame from '../components/PageFrame.svelte';
  import TrackProgress from '../components/TrackProgress.svelte';
  import { dash, music } from '../state.svelte';

  const m = $derived(dash.music);
</script>

<PageFrame title="Muzyka" subtitle="Spotify">
  {#if m}
    <div class="player">
      <div class="art">
        {#if m.artUrl}<img src={m.artUrl} alt="" />{:else}<div class="ph"><Disc3 size={110} strokeWidth={1} /></div>{/if}
      </div>
      <div class="side">
        <div class="title">{m.title}</div>
        <div class="artist">{m.artist}</div>
        <div class="album">{m.album}</div>
        <div class="progress"><TrackProgress music={m} /></div>
        <div class="controls">
          <button aria-label="Poprzedni" onclick={() => music('previous')}><SkipBack size={38} fill="currentColor" /></button>
          <button class="main" aria-label={m.isPlaying ? 'Pauza' : 'Graj'} onclick={() => music(m.isPlaying ? 'pause' : 'play')}>
            {#if m.isPlaying}<Pause size={44} fill="currentColor" strokeWidth={0} />{:else}<Play size={44} fill="currentColor" strokeWidth={0} />{/if}
          </button>
          <button aria-label="Następny" onclick={() => music('next')}><SkipForward size={38} fill="currentColor" /></button>
        </div>
        <div class="device"><Speaker size={17} /> {m.device}</div>
      </div>
    </div>
  {:else}
    <p class="empty">Nic teraz nie gra</p>
  {/if}
</PageFrame>

<style>
  .player {
    display: flex;
    align-items: center;
    gap: 56px;
    height: 100%;
    padding: 0 24px;
  }

  .art {
    width: 520px;
    height: 520px;
    max-height: 100%;
    flex: none;
    border-radius: 26px;
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
    color: rgba(255, 255, 255, 0.7);
    background: linear-gradient(135deg, #5e5ce6, #ff375f 55%, #ff9f0a);
  }

  .side {
    flex: 1;
    min-width: 0;
  }

  .title {
    font-size: 40px;
    font-weight: 720;
    letter-spacing: -0.02em;
  }

  .artist {
    margin-top: 6px;
    font-size: 26px;
    color: var(--accent);
    font-weight: 560;
  }

  .album {
    margin-top: 4px;
    font-size: 18px;
    color: var(--muted);
  }

  .progress {
    margin-top: 38px;
  }

  .controls {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 40px;
    margin-top: 30px;
  }

  .controls button {
    display: grid;
    place-items: center;
    width: 76px;
    height: 76px;
    border-radius: 50%;
  }

  .controls .main {
    width: 104px;
    height: 104px;
    background: var(--accent);
    color: var(--accent-ink);
  }

  .controls button:active {
    transform: scale(0.92);
  }

  .device {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    margin-top: 30px;
    font-size: 15px;
    color: var(--muted);
  }

  .empty {
    color: var(--faint);
  }
</style>

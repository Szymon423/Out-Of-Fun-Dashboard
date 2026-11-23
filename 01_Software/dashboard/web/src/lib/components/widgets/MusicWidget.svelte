<!-- Now playing, like the player on the iPhone lock screen: a large cover with the title and
     artist beside it, underneath the progress bar with the controls between the two times.
     The controls are plain glyphs, so the cover stays the only colour. A tap elsewhere opens
     the music page. -->
<script lang="ts">
  import Disc3 from '@lucide/svelte/icons/disc-3';
  import Music from '@lucide/svelte/icons/music';
  import Pause from '@lucide/svelte/icons/pause';
  import Play from '@lucide/svelte/icons/play';
  import SkipBack from '@lucide/svelte/icons/skip-back';
  import SkipForward from '@lucide/svelte/icons/skip-forward';
  import Speaker from '@lucide/svelte/icons/speaker';
  import { dash, music } from '../../state.svelte';
  import TrackProgress from '../TrackProgress.svelte';
  import Widget from '../Widget.svelte';

  const m = $derived(dash.music);
</script>

<Widget to="music" class="music">
  {#if m}
    <div class="player">
      <div class="now">
        <div class="art" class:paused={!m.isPlaying}>
          {#if m.artUrl}<img src={m.artUrl} alt="" />{:else}<div class="ph"><Disc3 size={40} strokeWidth={1.3} /></div>{/if}
        </div>
        <div class="info">
          <div class="device"><Speaker size={14} /> <span>{m.device || 'Spotify'}</span></div>
          <div class="names">
            <div class="title">{m.title}</div>
            <div class="artist">{m.artist}</div>
          </div>
        </div>
      </div>

      <TrackProgress music={m}>
        {#snippet middle()}
          <div class="controls">
            <button aria-label="Poprzedni" onclick={() => music('previous')}>
              <SkipBack size={26} fill="currentColor" strokeWidth={1.5} />
            </button>
            <button class="main" aria-label={m.isPlaying ? 'Pauza' : 'Graj'} onclick={() => music(m.isPlaying ? 'pause' : 'play')}>
              {#if m.isPlaying}
                <Pause size={34} fill="currentColor" strokeWidth={0} />
              {:else}
                <Play size={34} fill="currentColor" strokeWidth={0} />
              {/if}
            </button>
            <button aria-label="Następny" onclick={() => music('next')}>
              <SkipForward size={26} fill="currentColor" strokeWidth={1.5} />
            </button>
          </div>
        {/snippet}
      </TrackProgress>
    </div>
  {:else}
    <div class="empty">
      <Music size={30} strokeWidth={1.5} />
      <span>Nic teraz nie gra</span>
    </div>
  {/if}
</Widget>

<style>
  .player {
    display: flex;
    flex: 1;
    flex-direction: column;
    justify-content: space-between;
    gap: 14px;
    min-height: 0;
  }

  .now {
    display: flex;
    gap: 18px;
    min-height: 0;
  }

  .art {
    width: 158px;
    height: 158px;
    flex: none;
    border-radius: 16px;
    overflow: hidden;
    box-shadow: 0 0 0 1px var(--line);
    transition: opacity 0.4s ease;
  }

  .art.paused {
    opacity: 0.6;
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
    color: rgba(255, 255, 255, 0.8);
    background: linear-gradient(135deg, #5e5ce6, #ff375f 55%, #ff9f0a);
  }

  /* Device at the top, title and artist down by the bottom edge of the cover. */
  .info {
    display: flex;
    flex: 1;
    flex-direction: column;
    justify-content: space-between;
    min-width: 0;
    padding: 2px 0 4px;
  }

  .device {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 13.5px;
    color: var(--muted);
  }

  .device span {
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .title {
    display: -webkit-box;
    -webkit-box-orient: vertical;
    -webkit-line-clamp: 2;
    line-clamp: 2;
    overflow: hidden;
    font-size: 21px;
    font-weight: 680;
    line-height: 1.2;
    letter-spacing: -0.01em;
  }

  .artist {
    margin-top: 3px;
    font-size: 16px;
    color: var(--muted);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .controls {
    display: flex;
    align-items: center;
    gap: 14px;
  }

  .controls button {
    display: grid;
    place-items: center;
    width: 46px;
    height: 46px;
    border-radius: 50%;
    color: var(--text);
  }

  .controls .main {
    width: 54px;
    height: 54px;
  }

  .controls button:active {
    transform: scale(0.88);
    background: var(--fill);
  }

  .empty {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 10px;
    font-size: 16px;
    color: var(--faint);
  }
</style>

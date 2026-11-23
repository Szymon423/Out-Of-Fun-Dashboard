<!--
  Music. Left: what plays, with the controls, the device it plays on (a tap opens the device
  picker) and its volume. Right: the library, as covers four by two, paged: the liked songs and
  the playlists, or the albums played lately. A tap on a cover plays it; when nothing is active
  yet, the picker asks where to play first.
-->
<script lang="ts">
  import ChevronDown from '@lucide/svelte/icons/chevron-down';
  import ChevronLeft from '@lucide/svelte/icons/chevron-left';
  import ChevronRight from '@lucide/svelte/icons/chevron-right';
  import Disc3 from '@lucide/svelte/icons/disc-3';
  import Heart from '@lucide/svelte/icons/heart';
  import Laptop from '@lucide/svelte/icons/laptop';
  import Music from '@lucide/svelte/icons/music';
  import Pause from '@lucide/svelte/icons/pause';
  import Play from '@lucide/svelte/icons/play';
  import SkipBack from '@lucide/svelte/icons/skip-back';
  import SkipForward from '@lucide/svelte/icons/skip-forward';
  import Smartphone from '@lucide/svelte/icons/smartphone';
  import Speaker from '@lucide/svelte/icons/speaker';
  import Tv from '@lucide/svelte/icons/tv';
  import Volume2 from '@lucide/svelte/icons/volume-2';
  import type { Component } from 'svelte';
  import PageFrame from '../components/PageFrame.svelte';
  import Segmented from '../components/Segmented.svelte';
  import Slider from '../components/Slider.svelte';
  import TrackProgress from '../components/TrackProgress.svelte';
  import { nav, PAGES } from '../nav.svelte';
  import { dash, fetchDevices, music, playContext, setVolume, transferTo } from '../state.svelte';
  import type { LibraryItem, SpotifyDevice } from '../types';

  const PAGE = 8;
  const m = $derived(dash.music);
  const lib = $derived(dash.musicLibrary);

  let tab = $state<'playlists' | 'recent'>('playlists');
  let page = $state(0);
  const TABS = [
    { value: 'playlists' as const, label: 'Playlisty' },
    { value: 'recent' as const, label: 'Ostatnio grane' },
  ];

  const items = $derived.by((): LibraryItem[] => {
    if (!lib) return [];
    if (tab === 'recent') return lib.recent;
    const liked = lib.likedUri ? [{ name: 'Polubione utwory', uri: lib.likedUri, imageUrl: null, detail: 'Twoja kolekcja' }] : [];
    return [...liked, ...lib.playlists];
  });
  const pages = $derived(Math.max(1, Math.ceil(items.length / PAGE)));
  const shown = $derived(items.slice(page * PAGE, page * PAGE + PAGE));

  function switchTab(value: 'playlists' | 'recent'): void {
    tab = value;
    page = 0;
  }

  /** A soft two-colour gradient from the name, for covers Spotify does not have. */
  function placeholder(name: string): string {
    let h = 0;
    for (const c of name) h = (h * 31 + c.charCodeAt(0)) % 360;
    return `linear-gradient(135deg, hsl(${h} 62% 52%), hsl(${(h + 48) % 360} 70% 38%))`;
  }

  // The device picker: to move the music elsewhere, or to choose where a cover should play.
  let picker = $state<{ devices: SpotifyDevice[] | null; play: string | null } | null>(null);

  async function openPicker(play: string | null = null): Promise<void> {
    picker = { devices: null, play };
    const devices = await fetchDevices();
    if (picker) picker.devices = devices;
  }

  function choose(device: SpotifyDevice): void {
    if (picker?.play) playContext(picker.play, device.id);
    else if (!device.isActive) transferTo(device.id);
    picker = null;
  }

  function play(item: LibraryItem): void {
    if (m) playContext(item.uri);
    else void openPicker(item.uri); // nothing active: ask where
  }

  const ICON: Record<string, Component> = { Computer: Laptop, Smartphone, TV: Tv, Speaker };

  // Leaving the page closes the picker.
  $effect(() => {
    if (picker && PAGES[nav.index]?.id !== 'music') picker = null;
  });
</script>

<PageFrame title="Muzyka" subtitle="Spotify">
  <div class="layout">
    <section class="now card">
      {#if m}
        <div class="art">
          {#if m.artUrl}<img src={m.artUrl} alt="" />{:else}<div class="ph"><Disc3 size={80} strokeWidth={1} /></div>{/if}
        </div>
        <div class="title">{m.title}</div>
        <div class="artist">{m.artist}</div>
        <div class="progress"><TrackProgress music={m} /></div>
        <div class="controls">
          <button aria-label="Poprzedni" onclick={() => music('previous')}><SkipBack size={32} fill="currentColor" /></button>
          <button class="main" aria-label={m.isPlaying ? 'Pauza' : 'Graj'} onclick={() => music(m.isPlaying ? 'pause' : 'play')}>
            {#if m.isPlaying}<Pause size={42} fill="currentColor" strokeWidth={0} />{:else}<Play size={42} fill="currentColor" strokeWidth={0} />{/if}
          </button>
          <button aria-label="Następny" onclick={() => music('next')}><SkipForward size={32} fill="currentColor" /></button>
        </div>
        <div class="bottom">
          <button class="device" onclick={() => openPicker()}>
            <Speaker size={17} /> <span>{m.device || 'Wybierz urządzenie'}</span> <ChevronDown size={16} />
          </button>
          {#if m.volume !== null}
            <div class="volume"><Slider value={m.volume} icon={Volume2} label="Głośność" onchange={setVolume} /></div>
          {/if}
        </div>
      {:else}
        <div class="idle">
          <Music size={44} strokeWidth={1.4} />
          <b>Nic teraz nie gra</b>
          <span>Wybierz playlistę obok albo urządzenie</span>
          <button class="device" onclick={() => openPicker()}><Speaker size={17} /> Urządzenia <ChevronDown size={16} /></button>
        </div>
      {/if}
    </section>

    <section class="library card">
      <header>
        <Segmented options={TABS} value={tab} onchange={switchTab} />
        {#if pages > 1}
          <div class="pager num">
            <button aria-label="Poprzednia strona" disabled={page === 0} onclick={() => (page = Math.max(0, page - 1))}>
              <ChevronLeft size={22} />
            </button>
            <span>{page + 1} / {pages}</span>
            <button aria-label="Następna strona" disabled={page >= pages - 1} onclick={() => (page = Math.min(pages - 1, page + 1))}>
              <ChevronRight size={22} />
            </button>
          </div>
        {/if}
      </header>

      {#if !lib}
        <p class="note">Playlisty pojawią się po zalogowaniu Spotify z nowymi uprawnieniami.</p>
      {:else if shown.length === 0}
        <p class="note">Nic tu jeszcze nie ma.</p>
      {:else}
        <div class="grid">
          {#each shown as item (item.uri)}
            <button class="item" onclick={() => play(item)}>
              <span class="cover" style:background={item.imageUrl ? undefined : placeholder(item.name)}>
                {#if item.imageUrl}
                  <img src={item.imageUrl} alt="" />
                {:else if item.uri === lib.likedUri}
                  <Heart size={44} fill="currentColor" strokeWidth={0} />
                {:else}
                  <Music size={36} strokeWidth={1.6} />
                {/if}
              </span>
              <span class="name">{item.name}</span>
              <span class="detail">{item.detail}</span>
            </button>
          {/each}
        </div>
      {/if}
    </section>
  </div>

  {#if picker}
    <!-- svelte-ignore a11y_no_static_element_interactions -->
    <div class="scrim" data-noswipe onpointerdown={(e) => e.target === e.currentTarget && (picker = null)}>
      <div class="sheet">
        <div class="sheet-title">{picker.play ? 'Gdzie zagrać?' : 'Urządzenie'}</div>
        {#if picker.devices === null}
          <p class="note">Szukam urządzeń…</p>
        {:else if picker.devices.length === 0}
          <p class="note">Brak urządzeń. Otwórz Spotify na telefonie, komputerze albo głośniku.</p>
        {:else}
          {#each picker.devices as d (d.id)}
            {@const Icon = ICON[d.type] ?? Speaker}
            <button class="dev" class:active={d.isActive} onclick={() => choose(d)}>
              <Icon size={22} />
              <span>{d.name}</span>
              {#if d.isActive}<small>gra tutaj</small>{/if}
            </button>
          {/each}
        {/if}
      </div>
    </div>
  {/if}
</PageFrame>

<style>
  .layout {
    display: grid;
    grid-template-columns: 400px 1fr;
    gap: var(--gap);
    height: 100%;
  }

  .card {
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
  }

  .now {
    display: flex;
    flex-direction: column;
    padding: 22px 24px;
    min-height: 0;
  }

  .art {
    width: 196px;
    height: 196px;
    flex: none;
    border-radius: 18px;
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
    color: rgba(255, 255, 255, 0.8);
    background: linear-gradient(135deg, #5e5ce6, #ff375f 55%, #ff9f0a);
  }

  .title {
    margin-top: 18px;
    font-size: 24px;
    font-weight: 700;
    letter-spacing: -0.01em;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .artist {
    margin-top: 2px;
    font-size: 17px;
    color: var(--muted);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .progress {
    margin-top: 16px;
  }

  .controls {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 34px;
    margin-top: 10px;
  }

  .controls button {
    display: grid;
    place-items: center;
    width: 56px;
    height: 56px;
    border-radius: 50%;
  }

  .controls .main {
    width: 68px;
    height: 68px;
  }

  .controls button:active,
  .item:active {
    transform: scale(0.94);
  }

  .bottom {
    display: flex;
    flex-direction: column;
    gap: 12px;
    margin-top: auto;
  }

  .device {
    display: flex;
    align-items: center;
    align-self: flex-start;
    gap: 8px;
    max-width: 100%;
    padding: 8px 14px;
    border-radius: 999px;
    font-size: 15px;
    font-weight: 600;
    color: var(--good);
    background: color-mix(in srgb, var(--good) 14%, transparent);
  }

  .volume {
    display: flex;
  }

  .device span {
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .idle {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 8px;
    height: 100%;
    color: var(--faint);
    text-align: center;
  }

  .idle b {
    font-size: 20px;
    color: var(--text);
  }

  .idle .device {
    align-self: center;
    margin-top: 12px;
  }

  .library {
    display: flex;
    flex-direction: column;
    padding: 18px 22px 20px;
    min-height: 0;
  }

  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .pager {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 14px;
    color: var(--muted);
  }

  .pager button {
    display: grid;
    place-items: center;
    width: 40px;
    height: 40px;
    border-radius: 50%;
    background: var(--fill);
  }

  .pager button:disabled {
    opacity: 0.35;
  }

  .grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    grid-template-rows: repeat(2, 1fr);
    gap: 16px 18px;
    flex: 1;
    margin-top: 16px;
    min-height: 0;
  }

  .item {
    display: flex;
    flex-direction: column;
    min-width: 0;
    text-align: left;
  }

  .cover {
    display: grid;
    place-items: center;
    width: 100%;
    aspect-ratio: 1;
    max-height: 176px;
    border-radius: 14px;
    overflow: hidden;
    color: rgba(255, 255, 255, 0.9);
    box-shadow: 0 0 0 1px var(--line);
  }

  .cover img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .name {
    margin-top: 8px;
    font-size: 15px;
    font-weight: 600;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .detail {
    font-size: 13px;
    color: var(--muted);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .note {
    margin: auto;
    max-width: 380px;
    text-align: center;
    color: var(--faint);
  }

  /* Device picker, over the page; data-noswipe keeps a drag on it from turning the page. */
  .scrim {
    position: absolute;
    inset: 0;
    z-index: 5;
    display: grid;
    place-items: center;
    background: rgba(0, 0, 0, 0.45);
  }

  .sheet {
    display: flex;
    flex-direction: column;
    gap: 6px;
    width: 440px;
    padding: 20px;
    border-radius: 26px;
    background: var(--card-solid);
    box-shadow: 0 0 0 1px var(--line);
  }

  .sheet-title {
    margin: 0 0 8px 6px;
    font-size: 20px;
    font-weight: 700;
  }

  .dev {
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 14px 16px;
    border-radius: 16px;
    font-size: 17px;
    text-align: left;
    background: var(--fill);
  }

  .dev.active {
    color: var(--good);
    background: color-mix(in srgb, var(--good) 14%, transparent);
  }

  .dev small {
    margin-left: auto;
    font-size: 13px;
  }
</style>

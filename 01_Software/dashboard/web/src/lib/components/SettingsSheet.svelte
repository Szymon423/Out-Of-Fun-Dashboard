<!--
  Settings, opened with the gear in the control bar. First three big tiles, one per module
  (photos, screensaver, music), each with what is set now; a tap opens the module with big
  options and a way back:
    photos       how long each photo stays and in which order
    screensaver  the night screensaver (black, clock and date only) and its hours
    music        which playlists the music page shows: covers as on that page, a green frame
                 when shown, greyed out without the frame when hidden
  Kept by the server (settings.hpp), so they survive restarts and updates. Mounted next to the
  pages, like the event sheet, so the playlist grid can scroll. A tap beside the card closes
  it; so does the screensaver or the overview starting.
-->
<script lang="ts">
  import type { Component } from 'svelte';
  import Check from '@lucide/svelte/icons/check';
  import ChevronLeft from '@lucide/svelte/icons/chevron-left';
  import ChevronRight from '@lucide/svelte/icons/chevron-right';
  import Images from '@lucide/svelte/icons/images';
  import Lock from '@lucide/svelte/icons/lock';
  import Minus from '@lucide/svelte/icons/minus';
  import Music from '@lucide/svelte/icons/music';
  import Plus from '@lucide/svelte/icons/plus';
  import X from '@lucide/svelte/icons/x';
  import { fade } from 'svelte/transition';
  import { nav } from '../nav.svelte';
  import { dash, saveSettings, settings } from '../state.svelte';
  import Toggle from './Toggle.svelte';

  type Module = 'photos' | 'saver' | 'music';

  const s = $derived(settings());
  const playlists = $derived(dash.musicLibrary?.playlists ?? []);

  // ?settings=music opens a module at once, for checking.
  const startModule = new URLSearchParams(location.search).get('settings');
  let module = $state<Module | null>(
    startModule === 'photos' || startModule === 'saver' || startModule === 'music' ? startModule : null,
  );

  const INTERVALS = [
    { value: 10, label: '10 s' },
    { value: 20, label: '20 s' },
    { value: 30, label: '30 s' },
    { value: 60, label: '1 min' },
    { value: 120, label: '2 min' },
    { value: 300, label: '5 min' },
  ];
  const ORDERS = [
    { value: 'random' as const, label: 'Losowo', detail: 'Za każdym razem inne zdjęcia z całej biblioteki' },
    { value: 'newest' as const, label: 'Od najnowszych', detail: 'Po kolei, od ostatnio zrobionych wstecz' },
  ];

  const intervalLabel = $derived(INTERVALS.find((i) => i.value === s.photoIntervalS)?.label ?? `${s.photoIntervalS} s`);
  const shownCount = $derived(playlists.filter((p) => !s.hiddenPlaylists.includes(p.uri)).length);

  const MODULES = $derived<{ id: Module; title: string; icon: Component; color: string; summary: string }[]>([
    {
      id: 'photos',
      title: 'Zdjęcia',
      icon: Images,
      color: '#ff9f0a',
      summary: `${intervalLabel} · ${s.photoOrder === 'random' ? 'losowo' : 'od najnowszych'}`,
    },
    {
      id: 'saver',
      title: 'Wygaszacz',
      icon: Lock,
      color: '#5e5ce6',
      summary: s.nightEnabled ? `Nocny ${s.nightFrom}–${s.nightTo}` : 'Nocny wyłączony',
    },
    {
      id: 'music',
      title: 'Muzyka',
      icon: Music,
      color: '#30d158',
      summary: playlists.length ? `${shownCount} z ${playlists.length} playlist` : 'Brak playlist',
    },
  ]);
  const current = $derived(MODULES.find((m) => m.id === module) ?? null);

  /** "HH:MM" moved by `by` minutes, around the clock. */
  function shift(time: string, by: number): string {
    const [h, m] = time.split(':').map(Number);
    const t = (((h * 60 + m + by) % 1440) + 1440) % 1440;
    return `${String(Math.floor(t / 60)).padStart(2, '0')}:${String(t % 60).padStart(2, '0')}`;
  }

  function toggleShown(uri: string): void {
    const hidden = s.hiddenPlaylists.includes(uri);
    saveSettings({
      hiddenPlaylists: hidden ? s.hiddenPlaylists.filter((u) => u !== uri) : [...s.hiddenPlaylists, uri],
    });
  }

  /** The same soft gradient the music page gives covers Spotify does not have. */
  function placeholder(name: string): string {
    let h = 0;
    for (const c of name) h = (h * 31 + c.charCodeAt(0)) % 360;
    return `linear-gradient(135deg, hsl(${h} 62% 52%), hsl(${(h + 48) % 360} 70% 38%))`;
  }

  function close(): void {
    nav.settings = false;
    module = null;
  }

  $effect(() => {
    if (nav.settings && (nav.saver || nav.overview)) close();
  });
</script>

{#snippet stepper(label: string, key: 'nightFrom' | 'nightTo')}
  <div class="stepper">
    <span class="label">{label}</span>
    <div class="controls">
      <button aria-label="{label} wcześniej" onclick={() => saveSettings({ [key]: shift(s[key], -15) })}>
        <Minus size={28} strokeWidth={2.2} />
      </button>
      <span class="time num">{s[key]}</span>
      <button aria-label="{label} później" onclick={() => saveSettings({ [key]: shift(s[key], 15) })}>
        <Plus size={28} strokeWidth={2.2} />
      </button>
    </div>
  </div>
{/snippet}

{#if nav.settings}
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div class="scrim" onpointerdown={(e) => e.target === e.currentTarget && close()} transition:fade={{ duration: 160 }}>
    <div class="sheet" role="dialog" aria-label="Ustawienia">
      <header>
        {#if current}
          <button class="back" onclick={() => (module = null)}>
            <ChevronLeft size={26} strokeWidth={2.2} />Ustawienia
          </button>
          <h2 class="title">{current.title}</h2>
        {:else}
          <h2>Ustawienia</h2>
        {/if}
        <button class="close" aria-label="Zamknij" onclick={close}><X size={24} strokeWidth={2.2} /></button>
      </header>

      {#if !current}
        <div class="modules">
          {#each MODULES as m (m.id)}
            <button class="module" style:--c={m.color} onclick={() => (module = m.id)}>
              <span class="icon"><m.icon size={34} strokeWidth={1.9} /></span>
              <span class="name">{m.title}</span>
              <span class="summary">{m.summary}</span>
              <span class="go"><ChevronRight size={24} strokeWidth={2.2} /></span>
            </button>
          {/each}
        </div>
      {:else if module === 'photos'}
        <div class="body">
          <h3>Czas wyświetlania jednego zdjęcia</h3>
          <div class="choices six">
            {#each INTERVALS as i (i.value)}
              <button class="choice" class:on={s.photoIntervalS === i.value} onclick={() => saveSettings({ photoIntervalS: i.value })}>
                {i.label}
              </button>
            {/each}
          </div>
          <h3>Kolejność</h3>
          <div class="choices two">
            {#each ORDERS as o (o.value)}
              <button class="choice tall" class:on={s.photoOrder === o.value} onclick={() => saveSettings({ photoOrder: o.value })}>
                <span class="choice-title">{o.label}</span>
                <small>{o.detail}</small>
              </button>
            {/each}
          </div>
        </div>
      {:else if module === 'saver'}
        <div class="body">
          <div class="switch-row">
            <span>
              Wygaszacz nocny
              <small>W tych godzinach wygaszacz to czarny ekran z zegarem i datą na środku, bez zdjęć.</small>
            </span>
            <Toggle on={s.nightEnabled} label="Wygaszacz nocny" onchange={(on) => saveSettings({ nightEnabled: on })} />
          </div>
          <div class="hours" class:off={!s.nightEnabled}>
            {@render stepper('Od', 'nightFrom')}
            {@render stepper('Do', 'nightTo')}
          </div>
        </div>
      {:else if module === 'music'}
        <div class="body scroll">
          <p class="hint">Zaznaczone playlisty są widoczne na stronie muzyki.</p>
          {#if playlists.length}
            <div class="covers">
              {#each playlists as p (p.uri)}
                {@const shown = !s.hiddenPlaylists.includes(p.uri)}
                <button class="cover" class:shown aria-pressed={shown} onclick={() => toggleShown(p.uri)}>
                  <span class="art" style:background={p.imageUrl ? undefined : placeholder(p.name)}>
                    {#if p.imageUrl}<img src={p.imageUrl} alt="" />{:else}<Music size={34} strokeWidth={1.8} />{/if}
                    {#if shown}<i class="check"><Check size={18} strokeWidth={3} /></i>{/if}
                  </span>
                  <span class="cover-name">{p.name}</span>
                </button>
              {/each}
            </div>
          {:else}
            <p class="hint">Brak playlist ze Spotify.</p>
          {/if}
        </div>
      {/if}
    </div>
  </div>
{/if}

<style>
  .scrim {
    position: absolute;
    inset: 0;
    z-index: 8;
    display: grid;
    place-items: center;
    background: rgba(0, 0, 0, 0.45);
  }

  /* No big blurred shadow (docs/plan.md); the dimmed page sets the card apart. */
  .sheet {
    display: flex;
    flex-direction: column;
    width: 1000px;
    height: 620px;
    padding: 22px 0 0;
    border-radius: 30px;
    background: var(--card-solid);
    box-shadow: 0 0 0 1px var(--line);
    overflow: hidden;
    animation: rise 0.24s var(--ease);
  }

  @keyframes rise {
    from {
      opacity: 0;
      transform: translateY(14px) scale(0.98);
    }
  }

  header {
    position: relative;
    display: flex;
    align-items: center;
    justify-content: space-between;
    min-height: 56px;
    padding: 0 22px 0 32px;
  }

  h2 {
    font-size: 30px;
    font-weight: 700;
  }

  h2.title {
    position: absolute;
    left: 50%;
    transform: translateX(-50%);
    font-size: 24px;
  }

  .back {
    display: flex;
    align-items: center;
    gap: 4px;
    height: 56px;
    margin-left: -12px;
    padding: 0 20px 0 10px;
    border-radius: 28px;
    background: var(--fill-strong);
    font-size: 18px;
    font-weight: 600;
  }

  .close {
    display: grid;
    place-items: center;
    width: 56px;
    height: 56px;
    border-radius: 50%;
    background: var(--fill-strong);
  }

  .back:active,
  .close:active,
  .module:active,
  .choice:active,
  .cover:active,
  .stepper button:active {
    transform: scale(0.97);
  }

  /* The three modules. */
  .modules {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 20px;
    flex: 1;
    padding: 24px 32px 32px;
  }

  .module {
    position: relative;
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    padding: 28px;
    border-radius: 26px;
    background: var(--fill);
    box-shadow: 0 0 0 1px var(--line);
    text-align: left;
  }

  .module .icon {
    display: grid;
    place-items: center;
    width: 72px;
    height: 72px;
    border-radius: 20px;
    background: var(--c);
    color: #fff;
  }

  .module .name {
    margin-top: auto;
    font-size: 30px;
    font-weight: 700;
  }

  .module .summary {
    margin-top: 6px;
    font-size: 18px;
    font-weight: 520;
    color: var(--muted);
  }

  .module .go {
    position: absolute;
    top: 28px;
    right: 24px;
    color: var(--muted);
  }

  /* A module's options. */
  .body {
    flex: 1;
    padding: 12px 32px 32px;
  }

  .body.scroll {
    overflow-y: auto;
    touch-action: pan-y;
  }

  h3 {
    margin: 16px 0 12px;
    font-size: 15px;
    font-weight: 650;
    letter-spacing: 0.04em;
    text-transform: uppercase;
    color: var(--muted);
  }

  .choices {
    display: grid;
    gap: 14px;
  }

  .choices.six {
    grid-template-columns: repeat(6, 1fr);
  }

  .choices.two {
    grid-template-columns: repeat(2, 1fr);
  }

  .choice {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    min-height: 80px;
    border-radius: 20px;
    background: var(--fill);
    box-shadow: inset 0 0 0 2px transparent;
    font-size: 22px;
    font-weight: 650;
    color: var(--muted);
  }

  .choice.tall {
    align-items: flex-start;
    min-height: 120px;
    padding: 0 26px;
    text-align: left;
  }

  .choice small {
    margin-top: 6px;
    font-size: 15px;
    font-weight: 480;
  }

  .choice.on {
    background: color-mix(in srgb, var(--accent, #0a84ff) 16%, var(--fill));
    box-shadow: inset 0 0 0 2px var(--accent, #0a84ff);
    color: var(--text);
  }

  .switch-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 24px;
    padding: 24px 28px;
    border-radius: 22px;
    background: var(--fill);
    font-size: 22px;
    font-weight: 620;
  }

  .switch-row small {
    display: block;
    margin-top: 6px;
    font-size: 16px;
    font-weight: 460;
    color: var(--muted);
  }

  .hours {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 20px;
    margin-top: 20px;
    transition: opacity 0.2s ease;
  }

  .hours.off {
    opacity: 0.4;
  }

  .stepper {
    padding: 22px 28px 26px;
    border-radius: 22px;
    background: var(--fill);
  }

  .stepper .label {
    font-size: 17px;
    font-weight: 600;
    color: var(--muted);
  }

  .stepper .controls {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-top: 12px;
  }

  .stepper button {
    display: grid;
    place-items: center;
    width: 72px;
    height: 72px;
    border-radius: 50%;
    background: var(--fill-strong);
  }

  .stepper .time {
    font-size: 48px;
    font-weight: 620;
  }

  /* Playlists, as covers. */
  .hint {
    margin: 4px 0 16px;
    font-size: 17px;
    color: var(--muted);
  }

  .covers {
    display: grid;
    grid-template-columns: repeat(5, 1fr);
    gap: 20px 18px;
  }

  .cover {
    display: flex;
    flex-direction: column;
    gap: 10px;
    min-width: 0;
    text-align: left;
    opacity: 0.4;
    transition: opacity 0.2s ease;
  }

  .cover.shown {
    opacity: 1;
  }

  .art {
    position: relative;
    display: grid;
    place-items: center;
    aspect-ratio: 1;
    border-radius: 16px;
    color: #fff;
    background: var(--fill-strong);
    box-shadow: 0 0 0 4px transparent;
  }

  .shown .art {
    box-shadow: 0 0 0 4px var(--good, #30d158);
  }

  .art img {
    width: 100%;
    height: 100%;
    border-radius: 16px;
    object-fit: cover;
  }

  .check {
    position: absolute;
    top: 8px;
    right: 8px;
    display: grid;
    place-items: center;
    width: 28px;
    height: 28px;
    border-radius: 50%;
    background: var(--good, #30d158);
    color: #fff;
  }

  .cover-name {
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
    font-size: 16px;
    font-weight: 600;
  }
</style>

<!--
  Settings, opened with the gear in the control bar: how long each photo stays and in which
  order, the night screensaver with its hours, and the playlists hidden from the music page.
  Kept by the server (settings.hpp), so they survive restarts and updates. Mounted next to the
  pages, like the event sheet, so its list can scroll. A tap beside the card closes it; so
  does the screensaver or the overview starting.
-->
<script lang="ts">
  import Minus from '@lucide/svelte/icons/minus';
  import Plus from '@lucide/svelte/icons/plus';
  import X from '@lucide/svelte/icons/x';
  import { fade } from 'svelte/transition';
  import { nav } from '../nav.svelte';
  import { dash, saveSettings, settings } from '../state.svelte';
  import Segmented from './Segmented.svelte';
  import Toggle from './Toggle.svelte';

  const s = $derived(settings());

  const INTERVALS = [
    { value: '10', label: '10 s' },
    { value: '20', label: '20 s' },
    { value: '30', label: '30 s' },
    { value: '60', label: '1 min' },
    { value: '120', label: '2 min' },
    { value: '300', label: '5 min' },
  ];
  const ORDERS = [
    { value: 'random' as const, label: 'Losowo' },
    { value: 'newest' as const, label: 'Od najnowszych' },
  ];

  /** "HH:MM" moved by `by` minutes, around the clock. */
  function shift(time: string, by: number): string {
    const [h, m] = time.split(':').map(Number);
    const t = (((h * 60 + m + by) % 1440) + 1440) % 1440;
    return `${String(Math.floor(t / 60)).padStart(2, '0')}:${String(t % 60).padStart(2, '0')}`;
  }

  const playlists = $derived(dash.musicLibrary?.playlists ?? []);

  function setShown(uri: string, shown: boolean): void {
    const hidden = s.hiddenPlaylists.filter((u) => u !== uri);
    saveSettings({ hiddenPlaylists: shown ? hidden : [...hidden, uri] });
  }

  function close(): void {
    nav.settings = false;
  }

  $effect(() => {
    if (nav.settings && (nav.saver || nav.overview)) nav.settings = false;
  });
</script>

{#snippet stepper(label: string, key: 'nightFrom' | 'nightTo')}
  <div class="stepper">
    <span class="label">{label}</span>
    <button aria-label="{label} wcześniej" onclick={() => saveSettings({ [key]: shift(s[key], -15) })}>
      <Minus size={22} strokeWidth={2.2} />
    </button>
    <span class="time num">{s[key]}</span>
    <button aria-label="{label} później" onclick={() => saveSettings({ [key]: shift(s[key], 15) })}>
      <Plus size={22} strokeWidth={2.2} />
    </button>
  </div>
{/snippet}

{#if nav.settings}
  <!-- svelte-ignore a11y_no_static_element_interactions -->
  <div class="scrim" onpointerdown={(e) => e.target === e.currentTarget && close()} transition:fade={{ duration: 160 }}>
    <div class="sheet" role="dialog" aria-label="Ustawienia">
      <header>
        <h2>Ustawienia</h2>
        <button class="close" aria-label="Zamknij" onclick={close}><X size={22} strokeWidth={2.2} /></button>
      </header>

      <div class="body">
        <section>
          <h3>Zdjęcia</h3>
          <div class="row">
            <span>Czas wyświetlania</span>
            <Segmented
              options={INTERVALS}
              value={String(s.photoIntervalS)}
              onchange={(v) => saveSettings({ photoIntervalS: Number(v) })}
            />
          </div>
          <div class="row">
            <span>Kolejność</span>
            <Segmented options={ORDERS} value={s.photoOrder} onchange={(v) => saveSettings({ photoOrder: v })} />
          </div>
        </section>

        <section>
          <h3>Wygaszacz nocny</h3>
          <div class="row">
            <span>
              Czarny ekran z zegarem
              <small>W tych godzinach wygaszacz pokazuje tylko godzinę i datę.</small>
            </span>
            <Toggle on={s.nightEnabled} label="Wygaszacz nocny" onchange={(on) => saveSettings({ nightEnabled: on })} />
          </div>
          <div class="row hours" class:off={!s.nightEnabled}>
            {@render stepper('Od', 'nightFrom')}
            {@render stepper('Do', 'nightTo')}
          </div>
        </section>

        <section>
          <h3>Playlisty na stronie muzyki</h3>
          {#each playlists as p (p.uri)}
            <div class="row playlist">
              <span>{p.name}</span>
              <Toggle
                on={!s.hiddenPlaylists.includes(p.uri)}
                label={p.name}
                onchange={(on) => setShown(p.uri, on)}
              />
            </div>
          {:else}
            <p class="none">Brak playlist ze Spotify.</p>
          {/each}
        </section>
      </div>
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
    width: 820px;
    max-height: 640px;
    padding: 22px 0 8px;
    border-radius: 28px;
    background: var(--card-solid);
    box-shadow: 0 0 0 1px var(--line);
    animation: rise 0.24s var(--ease);
  }

  @keyframes rise {
    from {
      opacity: 0;
      transform: translateY(14px) scale(0.98);
    }
  }

  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 0 22px 8px 32px;
  }

  h2 {
    font-size: 28px;
    font-weight: 700;
  }

  .close {
    display: grid;
    place-items: center;
    width: 48px;
    height: 48px;
    border-radius: 50%;
    background: var(--fill-strong);
  }

  .body {
    overflow-y: auto;
    padding: 0 32px 20px;
    touch-action: pan-y;
  }

  section {
    margin-top: 18px;
  }

  h3 {
    margin-bottom: 4px;
    font-size: 14px;
    font-weight: 650;
    letter-spacing: 0.04em;
    text-transform: uppercase;
    color: var(--muted);
  }

  .row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 20px;
    min-height: 64px;
    border-bottom: 1px solid var(--line);
    font-size: 17px;
    font-weight: 560;
  }

  .row:last-child {
    border-bottom: none;
  }

  small {
    display: block;
    margin-top: 2px;
    font-size: 14px;
    font-weight: 450;
    color: var(--muted);
  }

  .hours {
    justify-content: flex-start;
    gap: 40px;
    transition: opacity 0.2s ease;
  }

  .hours.off {
    opacity: 0.4;
  }

  .stepper {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .stepper .label {
    width: 30px;
    color: var(--muted);
  }

  .stepper button {
    display: grid;
    place-items: center;
    width: 48px;
    height: 48px;
    border-radius: 50%;
    background: var(--fill-strong);
  }

  .stepper button:active {
    transform: scale(0.94);
  }

  .stepper .time {
    width: 72px;
    text-align: center;
    font-size: 22px;
    font-weight: 600;
  }

  .playlist span {
    overflow: hidden;
    white-space: nowrap;
    text-overflow: ellipsis;
  }

  .none {
    padding: 16px 0;
    color: var(--muted);
  }
</style>

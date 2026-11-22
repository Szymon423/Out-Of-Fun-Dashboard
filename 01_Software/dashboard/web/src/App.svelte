<!-- The 1280x720 screen: wallpaper, the pages (swipe, overview) and the screensaver on top.
     ?fonts shows the clock font candidates instead. -->
<script lang="ts">
  import FlaskConical from '@lucide/svelte/icons/flask-conical';
  import WifiOff from '@lucide/svelte/icons/wifi-off';
  import Backdrop from './lib/components/Backdrop.svelte';
  import ControlBar from './lib/components/ControlBar.svelte';
  import Pager from './lib/components/Pager.svelte';
  import Screensaver from './lib/components/Screensaver.svelte';
  import FontsPage from './lib/pages/FontsPage.svelte';
  import { nav } from './lib/nav.svelte';
  import { connection } from './lib/state.svelte';
  import { resolvedTheme } from './lib/theme.svelte';

  const fonts = new URLSearchParams(location.search).has('fonts');
  let covered = $state(false);

  $effect(() => {
    document.documentElement.dataset.theme = resolvedTheme();
  });
</script>

{#if fonts}
  <FontsPage />
{:else}
  <div class="world" class:hidden={covered && !nav.waking}>
    <Backdrop paused={covered} />
    <Pager />
    <ControlBar />
  </div>
  <Screensaver bind:covered />

  {#if connection.mode === 'mock'}
    <div class="status"><FlaskConical size={13} /> dane testowe</div>
  {:else if connection.mode === 'offline'}
    <div class="status warn"><WifiOff size={13} /> brak połączenia z serwerem</div>
  {/if}
{/if}

<style>
  .world {
    position: absolute;
    inset: 0;
  }

  /* Under a fully opaque screensaver nothing of the pages needs drawing. */
  .world.hidden {
    visibility: hidden;
  }

  .status {
    position: absolute;
    right: 14px;
    bottom: 6px;
    z-index: 20;
    display: inline-flex;
    align-items: center;
    gap: 5px;
    padding: 2px 9px;
    border-radius: 999px;
    font-size: 11px;
    color: var(--faint);
    background: var(--fill);
    pointer-events: none;
  }

  .status.warn {
    color: var(--bad);
  }
</style>

<!-- ?fonts: clock font candidates side by side on the real screen. A tap picks one; the
     whole dashboard then uses it (remembered in this browser). -->
<script lang="ts">
  import Check from '@lucide/svelte/icons/check';
  import { CLOCK_FONTS, chooseClockFont, clockFont } from '../clockFont.svelte';

  function done(): void {
    location.search = '';
  }
</script>

<div class="fonts">
  <header>
    <div>
      <h1>Krój zegara</h1>
      <p>Stuknij, żeby wybrać. Wybór zapamięta się na tym ekranie.</p>
    </div>
    <button class="done" onclick={done}>Gotowe</button>
  </header>
  <div class="grid">
    {#each CLOCK_FONTS as f (f.id)}
      <button class="sample" class:on={clockFont.id === f.id} onclick={() => chooseClockFont(f.id)}>
        <span class="digits num" style="{f.style};font-size:{Math.round(96 * f.scale)}px">21:47</span>
        <span class="name">
          {#if clockFont.id === f.id}<Check size={16} strokeWidth={2.6} />{/if}
          {f.label}
        </span>
      </button>
    {/each}
  </div>
</div>

<style>
  .fonts {
    position: absolute;
    inset: 0;
    padding: 26px var(--pad);
    background: var(--bg);
  }

  header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
  }

  h1 {
    margin: 0;
    font-size: 32px;
    font-weight: 720;
  }

  p {
    margin: 4px 0 0;
    color: var(--muted);
  }

  .done {
    padding: 12px 22px;
    border-radius: 14px;
    background: var(--accent);
    color: #fff;
    font-weight: 650;
  }

  .grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 16px;
    margin-top: 22px;
  }

  .sample {
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 6px;
    height: 262px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
    overflow: hidden;
  }

  .sample.on {
    box-shadow:
      0 0 0 3px var(--accent),
      var(--shadow);
  }

  .digits {
    line-height: 1;
    color: var(--text);
  }

  .name {
    display: flex;
    align-items: center;
    gap: 6px;
    font-size: 14px;
    font-weight: 600;
    color: var(--muted);
  }

  .sample.on .name {
    color: var(--accent);
  }
</style>

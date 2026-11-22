<!-- The three desk lights as round toggles, like Control Center, across the clock column.
     A lit lamp fills with the accent colour, the LED strip with its own colour. -->
<script lang="ts">
  import LampDesk from '@lucide/svelte/icons/lamp-desk';
  import Sparkles from '@lucide/svelte/icons/sparkles';
  import { dash, setLight } from '../../state.svelte';
</script>

<div class="dock">
  {#each dash.lights as light (light.id)}
    {@const Icon = light.kind === 'rgbw' ? Sparkles : LampDesk}
    <button
      class="light"
      class:on={light.on}
      style:--c={light.kind === 'rgbw' ? (light.color ?? 'var(--accent)') : 'var(--accent)'}
      onclick={() => setLight(light.id, { on: !light.on })}
    >
      <span class="circle"><Icon size={36} strokeWidth={1.7} /></span>
      <span class="name">{light.name}</span>
      <span class="state num">
        {#if !light.on}wył.{:else if light.kind === 'rgbw'}{light.brightness ?? 0}%{:else}wł.{/if}
      </span>
    </button>
  {/each}
</div>

<style>
  .dock {
    display: flex;
    justify-content: space-between;
    padding-right: 10px;
  }

  .light {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
    width: 124px;
  }

  .circle {
    display: grid;
    place-items: center;
    width: 100px;
    height: 100px;
    border-radius: 50%;
    background: var(--card);
    box-shadow: var(--shadow);
    color: var(--muted);
    transition:
      background-color 0.25s ease,
      color 0.25s ease,
      transform 0.12s ease;
  }

  .light:active .circle {
    transform: scale(0.92);
  }

  .light.on .circle {
    background: var(--c);
    color: #fff;
  }

  .name {
    margin-top: 6px;
    font-size: 16px;
    font-weight: 600;
    white-space: nowrap;
  }

  .state {
    font-size: 13.5px;
    color: var(--faint);
  }

  .light.on .state {
    color: var(--accent);
  }
</style>

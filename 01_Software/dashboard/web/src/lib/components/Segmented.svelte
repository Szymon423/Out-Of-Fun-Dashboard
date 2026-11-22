<!-- iOS-style segmented control. -->
<script lang="ts" generics="T extends string">
  import type { Component } from 'svelte';

  let {
    options,
    value,
    onchange,
  }: { options: { value: T; label: string; icon?: Component }[]; value: T; onchange: (v: T) => void } = $props();
</script>

<div class="segmented" data-interactive>
  {#each options as o (o.value)}
    <button class:on={o.value === value} onclick={() => onchange(o.value)}>
      {#if o.icon}<o.icon size={16} strokeWidth={2} />{/if}
      {o.label}
    </button>
  {/each}
</div>

<style>
  .segmented {
    display: flex;
    padding: 3px;
    border-radius: 12px;
    background: var(--fill-strong);
  }

  button {
    display: flex;
    align-items: center;
    gap: 7px;
    min-height: 40px;
    padding: 0 16px;
    border-radius: 9px;
    font-size: 15px;
    font-weight: 560;
    color: var(--muted);
  }

  button.on {
    background: var(--card-solid);
    color: var(--text);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.14);
  }
</style>

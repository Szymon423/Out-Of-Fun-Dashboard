<!-- A translucent card with a small header. No backdrop-filter: blurring the moving
     background on every frame is too expensive on the Pi (docs/plan.md). -->
<script lang="ts">
  import type { Component, Snippet } from 'svelte';

  let {
    title,
    icon: Icon,
    aside,
    children,
    class: extra = '',
  }: { title: string; icon: Component; aside?: Snippet; children: Snippet; class?: string } = $props();
</script>

<section class="card {extra}">
  <header>
    <Icon size={15} strokeWidth={2.2} />
    <span>{title}</span>
    {#if aside}<div class="aside">{@render aside()}</div>{/if}
  </header>
  {@render children()}
</section>

<style>
  .card {
    position: relative;
    display: flex;
    flex-direction: column;
    min-height: 0;
    padding: 18px 22px 20px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
  }

  header {
    display: flex;
    align-items: center;
    gap: 8px;
    margin-bottom: 14px;
    color: var(--faint);
    font-size: 12.5px;
    font-weight: 600;
    letter-spacing: 0.12em;
    text-transform: uppercase;
  }

  .aside {
    margin-left: auto;
    letter-spacing: 0.02em;
    text-transform: none;
    font-weight: 500;
    font-size: 13px;
    color: var(--muted);
  }
</style>

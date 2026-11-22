<!-- A page with an iOS-style large title, an optional subtitle and controls on the right. -->
<script lang="ts">
  import type { Snippet } from 'svelte';

  let {
    title,
    subtitle = '',
    actions,
    children,
  }: { title: string; subtitle?: string; actions?: Snippet; children: Snippet } = $props();
</script>

<div class="frame">
  <header>
    <h1>{title}</h1>
    {#if subtitle}<span class="sub">{subtitle}</span>{/if}
    {#if actions}<div class="actions" data-interactive>{@render actions()}</div>{/if}
  </header>
  <div class="body">{@render children()}</div>
</div>

<style>
  .frame {
    display: flex;
    flex-direction: column;
    height: 100%;
    padding: 26px var(--pad) var(--pad);
  }

  header {
    display: flex;
    align-items: center;
    gap: 14px;
    min-height: 46px;
    margin-bottom: 16px;
  }

  h1 {
    margin: 0;
    font-size: 34px;
    font-weight: 720;
    letter-spacing: -0.02em;
  }

  .sub {
    font-size: 17px;
    color: var(--muted);
  }

  .actions {
    display: flex;
    align-items: center;
    gap: 12px;
    margin-left: auto;
  }

  .body {
    flex: 1;
    min-height: 0;
  }
</style>

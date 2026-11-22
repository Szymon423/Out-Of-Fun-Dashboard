<!-- An iOS-style widget: rounded, quiet, no visible header frame. With `to`, a tap on an
     empty part opens that page (buttons inside keep their own taps). -->
<script lang="ts">
  import type { Snippet } from 'svelte';
  import { go, type PageId } from '../nav.svelte';

  let {
    to,
    children,
    class: extra = '',
    flush = false,
  }: { to?: PageId; children: Snippet; class?: string; flush?: boolean } = $props();

  function open(e: MouseEvent): void {
    if (!to) return;
    if (e.target instanceof Element && e.target.closest('button')) return;
    go(to);
  }
</script>

<!-- svelte-ignore a11y_click_events_have_key_events, a11y_no_static_element_interactions -->
<section class="widget {extra}" class:flush class:link={!!to} onclick={open}>
  {@render children()}
</section>

<style>
  .widget {
    position: relative;
    min-height: 0;
    padding: 20px 22px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
    overflow: hidden;
  }

  .widget.flush {
    padding: 0;
  }

  .widget.link:active {
    transform: scale(0.985);
    transition: transform 0.12s ease;
  }
</style>

<!-- iOS switch. The knob moves with a transform. -->
<script lang="ts">
  let { on, onchange, label = '' }: { on: boolean; onchange: (on: boolean) => void; label?: string } = $props();
</script>

<button class="toggle" class:on role="switch" aria-checked={on} aria-label={label} onclick={() => onchange(!on)}>
  <i></i>
</button>

<style>
  .toggle {
    position: relative;
    width: 62px;
    height: 36px;
    flex: none;
    border-radius: 999px;
    background: var(--fill-strong);
    transition: background-color 0.25s ease;
  }

  .toggle.on {
    background: var(--good);
  }

  i {
    position: absolute;
    left: 3px;
    top: 3px;
    width: 30px;
    height: 30px;
    border-radius: 50%;
    background: #fff;
    box-shadow: 0 2px 6px rgba(0, 0, 0, 0.25);
    transition: transform 0.25s var(--ease);
  }

  .toggle.on i {
    transform: translateX(26px);
  }
</style>

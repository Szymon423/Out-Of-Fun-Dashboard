<!--
  Wallpaper behind all pages: slowly drifting light blobs, coloured by the theme. Only
  transform is animated, so the compositor moves the layers without repainting them.
  `paused` stops the drift while the screensaver covers everything. ?still turns the drift
  off, to measure what it costs (src/lib/bench.ts).
-->
<script lang="ts">
  let { paused = false }: { paused?: boolean } = $props();
  const still = new URLSearchParams(location.search).has('still');
</script>

<div class="backdrop" class:paused class:still aria-hidden="true">
  <i class="blob warm"></i>
  <i class="blob violet"></i>
  <i class="blob teal"></i>
  <div class="vignette"></div>
</div>

<style>
  .backdrop {
    --base: radial-gradient(120% 90% at 18% 0%, #141a2c 0%, #050608 62%);
    --warm: rgba(255, 159, 10, 0.22);
    --violet: rgba(94, 92, 230, 0.3);
    --teal: rgba(100, 210, 255, 0.16);
    --vignette: rgba(0, 0, 0, 0.5);
    position: absolute;
    inset: 0;
    overflow: hidden;
    background: var(--base);
  }

  :global([data-theme='light']) .backdrop {
    --base: radial-gradient(120% 90% at 18% 0%, #f6f7fa 0%, #dfe3eb 62%);
    --warm: rgba(255, 170, 110, 0.38);
    --violet: rgba(160, 140, 255, 0.32);
    --teal: rgba(90, 200, 250, 0.28);
    --vignette: rgba(120, 130, 160, 0.12);
  }

  .blob {
    position: absolute;
    border-radius: 50%;
    will-change: transform;
    animation: drift 44s ease-in-out infinite alternate;
  }

  .paused .blob {
    animation-play-state: paused;
  }

  .still .blob {
    animation: none;
    will-change: auto;
  }

  .warm {
    width: 780px;
    height: 780px;
    left: -220px;
    top: -300px;
    background: radial-gradient(circle, var(--warm), transparent 68%);
  }

  .violet {
    width: 900px;
    height: 900px;
    right: -300px;
    top: -180px;
    background: radial-gradient(circle, var(--violet), transparent 68%);
    animation-duration: 56s;
    animation-delay: -14s;
  }

  .teal {
    width: 760px;
    height: 760px;
    left: 360px;
    bottom: -440px;
    background: radial-gradient(circle, var(--teal), transparent 68%);
    animation-duration: 50s;
    animation-delay: -26s;
  }

  .vignette {
    position: absolute;
    inset: 0;
    background: radial-gradient(120% 120% at 50% 40%, transparent 55%, var(--vignette));
  }

  @keyframes drift {
    from {
      transform: translate(0, 0) scale(1);
    }
    to {
      transform: translate(120px, 70px) scale(1.12);
    }
  }
</style>

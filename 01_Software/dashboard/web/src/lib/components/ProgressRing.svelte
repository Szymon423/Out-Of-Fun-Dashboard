<!-- Circular progress (Apple Activity style). The arc changes only when the value does,
     which for a print is about once a minute. -->
<script lang="ts">
  let {
    value,
    size = 120,
    stroke = 12,
    color = 'var(--good)',
  }: { value: number; size?: number; stroke?: number; color?: string } = $props();

  const r = $derived((size - stroke) / 2);
  const circumference = $derived(2 * Math.PI * r);
</script>

<svg width={size} height={size} viewBox="0 0 {size} {size}" aria-hidden="true">
  <circle cx={size / 2} cy={size / 2} {r} fill="none" stroke="var(--fill-strong)" stroke-width={stroke} />
  <circle
    cx={size / 2}
    cy={size / 2}
    {r}
    fill="none"
    stroke={color}
    stroke-width={stroke}
    stroke-linecap="round"
    stroke-dasharray={circumference}
    stroke-dashoffset={circumference * (1 - Math.max(0, Math.min(100, value)) / 100)}
    transform="rotate(-90 {size / 2} {size / 2})"
  />
</svg>

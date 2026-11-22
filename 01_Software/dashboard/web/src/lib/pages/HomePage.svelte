<!-- Home control: the two desk lamps (on/off only) and the RGBW LED strip with brightness,
     white channel and colour. A mock until the hardware exists. -->
<script lang="ts">
  import LampDesk from '@lucide/svelte/icons/lamp-desk';
  import Palette from '@lucide/svelte/icons/palette';
  import Sun from '@lucide/svelte/icons/sun';
  import SunDim from '@lucide/svelte/icons/sun-dim';
  import { HUE_TRACK, hexToHue, hueToHex, LED_PRESETS } from '../color';
  import PageFrame from '../components/PageFrame.svelte';
  import Slider from '../components/Slider.svelte';
  import Toggle from '../components/Toggle.svelte';
  import { dash, setLight } from '../state.svelte';

  const lamps = $derived(dash.lights.filter((l) => l.kind === 'switch'));
  const strip = $derived(dash.lights.find((l) => l.kind === 'rgbw'));
  const onCount = $derived(dash.lights.filter((l) => l.on).length);
</script>

<PageFrame title="Dom" subtitle={onCount ? `Włączone: ${onCount} z ${dash.lights.length}` : 'Wszystko wyłączone'}>
  <div class="layout">
    <div class="lamps">
      {#each lamps as lamp (lamp.id)}
        <button class="lamp" class:on={lamp.on} onclick={() => setLight(lamp.id, { on: !lamp.on })}>
          <i class="glow"></i>
          <span class="icon"><LampDesk size={44} strokeWidth={1.6} /></span>
          <span class="text">
            <span class="name">{lamp.name}</span>
            <span class="state">{lamp.on ? 'Włączona' : 'Wyłączona'}</span>
          </span>
        </button>
      {/each}
    </div>

    {#if strip}
      <section class="strip" class:on={strip.on} style:--led={strip.color ?? '#ffffff'}>
        <header>
          <div>
            <div class="name">{strip.name}</div>
            <div class="state num">{strip.on ? `RGBW · ${strip.brightness ?? 0}%` : 'Wyłączona'}</div>
          </div>
          <Toggle on={strip.on} label={strip.name} onchange={(on) => setLight(strip.id, { on })} />
        </header>

        <div class="preview" style:opacity={strip.on ? 0.35 + ((strip.brightness ?? 0) / 100) * 0.65 : 0.12}>
          <i class="bar"></i>
        </div>

        <div class="controls" class:disabled={!strip.on}>
          <div class="row">
            <span class="label"><SunDim size={17} /> Jasność</span>
            <Slider value={strip.brightness ?? 0} icon={Sun} label="Jasność" onchange={(brightness) => setLight(strip.id, { on: true, brightness })} />
          </div>
          <div class="row">
            <span class="label"><Sun size={17} /> Biel</span>
            <Slider value={strip.white ?? 0} label="Biel" onchange={(white) => setLight(strip.id, { on: true, white })} />
          </div>
          <div class="row">
            <span class="label"><Palette size={17} /> Kolor</span>
            <Slider
              value={hexToHue(strip.color ?? '#ff0000')}
              max={359}
              track={HUE_TRACK}
              label="Kolor"
              onchange={(hue) => setLight(strip.id, { on: true, color: hueToHex(hue) })}
            />
          </div>
          <div class="presets">
            {#each LED_PRESETS as [name, color, white] (name)}
              <button
                class="preset"
                class:active={strip.color === color && strip.white === white}
                style:--c={color}
                onclick={() => setLight(strip.id, { on: true, color, white })}
              >
                <i></i>
                <span>{name}</span>
              </button>
            {/each}
          </div>
        </div>
      </section>
    {/if}
  </div>
</PageFrame>

<style>
  .layout {
    display: grid;
    grid-template-columns: 380px 1fr;
    gap: var(--gap);
    height: 100%;
  }

  .lamps {
    display: grid;
    grid-template-rows: 1fr 1fr;
    gap: var(--gap);
  }

  .lamp {
    position: relative;
    display: flex;
    align-items: center;
    gap: 22px;
    padding: 0 28px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
    text-align: left;
    overflow: hidden;
  }

  .lamp:active {
    transform: scale(0.98);
  }

  .glow {
    position: absolute;
    inset: 0;
    background: radial-gradient(90% 120% at 0% 50%, var(--accent-soft), transparent 70%);
    opacity: 0;
    transition: opacity 0.35s ease;
  }

  .lamp.on .glow {
    opacity: 1;
  }

  .icon {
    position: relative;
    display: grid;
    place-items: center;
    width: 96px;
    height: 96px;
    flex: none;
    border-radius: 50%;
    background: var(--fill);
    color: var(--muted);
    transition:
      background-color 0.3s ease,
      color 0.3s ease;
  }

  .lamp.on .icon {
    background: var(--accent);
    color: #fff;
  }

  .text {
    position: relative;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .name {
    font-size: 23px;
    font-weight: 650;
  }

  .state {
    font-size: 16px;
    color: var(--muted);
  }

  .strip {
    display: flex;
    flex-direction: column;
    padding: 24px 28px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
  }

  .strip header {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .preview {
    position: relative;
    margin: 20px 0 22px;
    transition: opacity 0.3s ease;
  }

  /* The glow around the bar: a gradient, cheap to paint (a blurred shadow is not, and this
     one repaints while a slider moves). */
  .preview::before {
    content: '';
    position: absolute;
    inset: -20px -6px;
    border-radius: 30px;
    background: linear-gradient(transparent, color-mix(in srgb, var(--led) 55%, transparent) 50%, transparent);
  }

  .bar {
    position: relative;
    display: block;
    height: 14px;
    border-radius: 7px;
    background: var(--led);
  }

  .controls {
    display: flex;
    flex-direction: column;
    gap: 14px;
    transition: opacity 0.3s ease;
  }

  .controls.disabled {
    opacity: 0.45;
  }

  .row {
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .label {
    display: flex;
    align-items: center;
    gap: 8px;
    width: 104px;
    flex: none;
    font-size: 15px;
    font-weight: 560;
    color: var(--muted);
  }

  .presets {
    display: grid;
    grid-template-columns: repeat(8, 1fr);
    gap: 8px;
    margin-top: 6px;
  }

  .preset {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 6px;
    padding: 8px 0;
    border-radius: 14px;
  }

  .preset i {
    width: 40px;
    height: 40px;
    border-radius: 50%;
    background: var(--c);
  }

  .preset span {
    font-size: 11.5px;
    color: var(--muted);
    white-space: nowrap;
  }

  .preset.active {
    background: var(--fill);
  }

  .preset.active i {
    box-shadow:
      0 0 0 3px var(--card-solid),
      0 0 0 5px var(--c);
  }
</style>

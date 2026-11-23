<!-- Home control: the two desk lamps (on/off only) and the RGBW LED strip, in the manner of
     Apple Home: two tall pills for brightness (filled in the strip's colour, so it shows the
     light) and the white channel, large colour swatches and a hue strip for any other colour.
     Changing a value switches the strip on. A soft glow in its colour fills the panel while
     it shines. A mock until the hardware exists. -->
<script lang="ts">
  import Lamp from '@lucide/svelte/icons/lamp';
  import LampDesk from '@lucide/svelte/icons/lamp-desk';
  import Sparkles from '@lucide/svelte/icons/sparkles';
  import Sun from '@lucide/svelte/icons/sun';
  import { HUE_TRACK, hexToHue, hueToHex, LED_PRESETS } from '../color';
  import PageFrame from '../components/PageFrame.svelte';
  import PillSlider from '../components/PillSlider.svelte';
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
      {@const led = strip.color ?? '#ffffff'}
      <section class="strip" class:on={strip.on} style:--led={led}>
        <i class="ambient" style:opacity={strip.on ? 0.35 + ((strip.brightness ?? 0) / 100) * 0.65 : 0}></i>
        <header>
          <span class="badge"><Sparkles size={26} strokeWidth={1.8} /></span>
          <div class="title">
            <div class="name">{strip.name}</div>
            <div class="state num">{strip.on ? `Włączona · ${strip.brightness ?? 0}%` : 'Wyłączona'}</div>
          </div>
          <Toggle on={strip.on} label={strip.name} onchange={(on) => setLight(strip.id, { on })} />
        </header>

        <div class="body">
          <div class="pills">
            <PillSlider
              label="Jasność"
              icon={Sun}
              value={strip.brightness ?? 0}
              fill="linear-gradient(to top, color-mix(in srgb, {led} 55%, #000), {led})"
              dim={!strip.on}
              onchange={(brightness) => setLight(strip.id, { on: true, brightness })}
            />
            <PillSlider
              label="Biel"
              icon={Lamp}
              value={strip.white ?? 0}
              fill="linear-gradient(to top, #f4b56a, #fff3dc)"
              dim={!strip.on}
              onchange={(white) => setLight(strip.id, { on: true, white })}
            />
          </div>

          <div class="colors">
            <div class="label">Kolor</div>
            <div class="swatches">
              {#each LED_PRESETS as [name, color, white] (name)}
                <button
                  class="swatch"
                  class:active={strip.color === color && strip.white === white}
                  style:--c={color}
                  onclick={() => setLight(strip.id, { on: true, color, white })}
                >
                  <i></i>
                  <span>{name}</span>
                </button>
              {/each}
            </div>
            <div class="label">Dowolny odcień</div>
            <div class="hue">
              <Slider
                value={hexToHue(led)}
                max={359}
                track={HUE_TRACK}
                height={42}
                label="Odcień"
                onchange={(hue) => setLight(strip.id, { on: true, color: hueToHex(hue) })}
              />
            </div>
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
    position: relative;
    display: flex;
    flex-direction: column;
    padding: 24px 28px 26px;
    border-radius: var(--radius);
    background: var(--card);
    box-shadow: var(--shadow);
    overflow: hidden;
  }

  /* The strip's light falling on the panel; a gradient, cheap to paint. */
  .ambient {
    position: absolute;
    inset: 0;
    background: radial-gradient(70% 80% at 12% 85%, color-mix(in srgb, var(--led) 26%, transparent), transparent 70%);
    transition: opacity 0.4s ease;
    pointer-events: none;
  }

  header {
    position: relative;
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .badge {
    display: grid;
    place-items: center;
    width: 56px;
    height: 56px;
    border-radius: 50%;
    color: var(--muted);
    background: var(--fill);
    transition:
      background-color 0.3s ease,
      color 0.3s ease;
  }

  .strip.on .badge {
    color: #fff;
    background: var(--led);
  }

  .title {
    flex: 1;
  }

  .body {
    position: relative;
    display: flex;
    flex: 1;
    gap: 44px;
    min-height: 0;
    margin-top: 22px;
  }

  .pills {
    display: flex;
    gap: 20px;
  }

  .colors {
    display: flex;
    flex: 1;
    flex-direction: column;
    justify-content: center;
    min-width: 0;
  }

  .label {
    margin-bottom: 14px;
    font-size: 13px;
    font-weight: 650;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--faint);
  }

  .swatches {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 18px 8px;
    margin-bottom: 30px;
  }

  .swatch {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 8px;
  }

  .swatch i {
    width: 66px;
    height: 66px;
    border-radius: 50%;
    background: var(--c);
    box-shadow: 0 0 0 1px var(--line);
    transition: box-shadow 0.2s ease;
  }

  .swatch.active i {
    box-shadow:
      0 0 0 4px var(--card-solid),
      0 0 0 7px var(--c);
  }

  .swatch:active i {
    transform: scale(0.92);
  }

  /* A row, so the slider keeps its own height instead of filling the column. */
  .hue {
    display: flex;
    padding: 0 4px;
  }

  .swatch span {
    font-size: 13px;
    color: var(--muted);
    white-space: nowrap;
  }

  .swatch.active span {
    color: var(--text);
    font-weight: 600;
  }
</style>

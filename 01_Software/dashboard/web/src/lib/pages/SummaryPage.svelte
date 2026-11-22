<!-- The middle page: everything at a glance. Clock, weather and lights on the left; calendar,
     Immich, music and the printer as widgets that open their own pages. -->
<script lang="ts">
  import ClockHero from '../components/widgets/ClockHero.svelte';
  import EventsWidget from '../components/widgets/EventsWidget.svelte';
  import LightsDock from '../components/widgets/LightsDock.svelte';
  import MusicWidget from '../components/widgets/MusicWidget.svelte';
  import PhotoWidget from '../components/widgets/PhotoWidget.svelte';
  import PrinterWidget from '../components/widgets/PrinterWidget.svelte';
</script>

<div class="summary">
  <div class="left">
    <ClockHero />
    <LightsDock />
  </div>
  <div class="grid">
    <div class="events"><EventsWidget /></div>
    <div class="photo"><PhotoWidget /></div>
    <div class="music"><MusicWidget /></div>
    <div class="printer"><PrinterWidget /></div>
  </div>
</div>

<style>
  .summary {
    display: grid;
    grid-template-columns: 430px 1fr;
    gap: 28px;
    height: 100%;
    padding: 30px var(--pad) var(--pad);
  }

  .left {
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    padding: 4px 0 2px 4px;
  }

  .grid {
    display: grid;
    grid-template-columns: 1fr 262px;
    grid-template-rows: 1.3fr 1fr;
    grid-template-areas:
      'events photo'
      'music printer';
    gap: var(--gap);
    min-height: 0;
  }

  .grid > div {
    display: grid;
    min-height: 0;
  }

  .events {
    grid-area: events;
  }
  .photo {
    grid-area: photo;
  }
  .music {
    grid-area: music;
  }
  .printer {
    grid-area: printer;
  }

  .music :global(.widget) {
    display: flex;
    flex-direction: column;
    justify-content: center;
  }
</style>

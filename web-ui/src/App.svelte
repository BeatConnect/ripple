<script lang="ts">
  import { onMount } from 'svelte';
  import ThreeBackground from './components/ThreeBackground.svelte';
  import RippleVisualizer from './components/RippleVisualizer.svelte';
  import Knob from './components/Knob.svelte';
  import Slider from './components/Slider.svelte';
  import Toggle from './components/Toggle.svelte';
  import Select from './components/Select.svelte';
  import LFOPanel from './components/LFOPanel.svelte';
  import ModMatrix from './components/ModMatrix.svelte';
  import Meter from './components/Meter.svelte';
  import {
    createSliderStore,
    createToggleStore,
    createComboStore,
    visualizerData
  } from './stores/params';
  import { emitEvent } from './lib/juce-bridge';

  // Ripple Filter Parameters
  const rippleRate = createSliderStore('ripple_rate', 0.25);
  const rippleMultiply = createSliderStore('ripple_multiply', 0.5);
  const rippleAmount = createSliderStore('ripple_amount', 0.5);
  const rippleWidth = createSliderStore('ripple_width', 0.75);
  const rippleLowBypass = createSliderStore('ripple_low_bypass', 0.1);
  const rippleHighBypass = createSliderStore('ripple_high_bypass', 0.8);
  const rippleMix = createSliderStore('ripple_mix', 1.0);

  // Reverb Parameters
  const reverbEnabled = createToggleStore('reverb_enabled', true);
  const reverbSize = createSliderStore('reverb_size', 0.5);
  const reverbDamping = createSliderStore('reverb_damping', 0.5);
  const reverbMix = createSliderStore('reverb_mix', 0.3);

  // Global Parameters
  const bypass = createToggleStore('bypass', false);
  const inputGain = createSliderStore('input_gain', 0.5);
  const outputGain = createSliderStore('output_gain', 0.5);

  function handleRandomize() {
    emitEvent('randomize', {});
  }
</script>

<ThreeBackground />

<div class="app-container">
  <!-- Header -->
  <header class="header">
    <div class="logo">
      <span class="logo-text">RIPPLE</span>
      <span class="logo-subtitle">TEXTURE ENGINE</span>
    </div>
    <div class="header-controls">
      <Toggle label="BYPASS" store={bypass} />
      <button class="btn btn-primary" on:click={handleRandomize}>
        RANDOMIZE
      </button>
    </div>
  </header>

  <!-- Main Content -->
  <main class="main">
    <!-- Left Panel: Visualizer -->
    <section class="visualizer-section panel">
      <RippleVisualizer bands={$visualizerData.rippleBands} lfoValues={$visualizerData.lfoValues} />
    </section>

    <!-- Center Panel: Ripple Filter -->
    <section class="ripple-section panel">
      <div class="panel-header">
        <span class="panel-title">RIPPLE FILTER</span>
      </div>
      <div class="panel-content">
        <div class="knob-row">
          <Knob
            label="RATE"
            store={rippleRate}
            min={0.01}
            max={20}
            unit="Hz"
            skew={0.3}
          />
          <Knob
            label="MULTIPLY"
            store={rippleMultiply}
            min={0}
            max={2}
          />
          <Knob
            label="RIPPLE"
            store={rippleAmount}
            min={0}
            max={2}
          />
          <Knob
            label="WIDTH"
            store={rippleWidth}
            min={0}
            max={2}
          />
        </div>
        <div class="slider-row">
          <Slider
            label="LOW BYPASS"
            store={rippleLowBypass}
            min={20}
            max={500}
            unit="Hz"
          />
          <Slider
            label="HIGH BYPASS"
            store={rippleHighBypass}
            min={1000}
            max={20000}
            unit="Hz"
          />
          <Slider
            label="MIX"
            store={rippleMix}
            min={0}
            max={100}
            unit="%"
          />
        </div>
      </div>
    </section>

    <!-- Right Panel: Reverb & I/O -->
    <section class="right-section">
      <!-- Reverb -->
      <div class="reverb-panel panel">
        <div class="panel-header">
          <Toggle store={reverbEnabled} compact />
          <span class="panel-title">REVERB</span>
        </div>
        <div class="panel-content">
          <div class="knob-row compact">
            <Knob label="SIZE" store={reverbSize} size="small" />
            <Knob label="DAMP" store={reverbDamping} size="small" />
            <Knob label="MIX" store={reverbMix} size="small" />
          </div>
        </div>
      </div>

      <!-- I/O Section -->
      <div class="io-panel panel">
        <div class="panel-header">
          <span class="panel-title">I/O</span>
        </div>
        <div class="panel-content io-content">
          <div class="meter-group">
            <Meter label="IN" value={$visualizerData.inputLevel} />
            <Knob label="GAIN" store={inputGain} size="small" bipolar />
          </div>
          <div class="meter-group">
            <Meter label="OUT" value={$visualizerData.outputLevel} />
            <Knob label="GAIN" store={outputGain} size="small" bipolar />
          </div>
        </div>
      </div>
    </section>
  </main>

  <!-- Bottom Section: LFOs & Mod Matrix -->
  <footer class="footer">
    <LFOPanel />
    <ModMatrix />
  </footer>
</div>

<style>
  .header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: var(--spacing-md) var(--spacing-lg);
    background: var(--bg-secondary);
    border-bottom: 1px solid var(--border-subtle);
  }

  .logo {
    display: flex;
    flex-direction: column;
  }

  .logo-text {
    font-family: var(--font-display);
    font-size: 24px;
    font-weight: 700;
    letter-spacing: 0.15em;
    background: linear-gradient(135deg, var(--accent-cyan), var(--accent-magenta));
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }

  .logo-subtitle {
    font-family: var(--font-display);
    font-size: 9px;
    font-weight: 500;
    letter-spacing: 0.3em;
    color: var(--text-muted);
    margin-top: -2px;
  }

  .header-controls {
    display: flex;
    align-items: center;
    gap: var(--spacing-md);
  }

  .main {
    flex: 1;
    display: grid;
    grid-template-columns: 280px 1fr 200px;
    gap: var(--spacing-md);
    padding: var(--spacing-md);
    min-height: 0;
  }

  .visualizer-section {
    display: flex;
    flex-direction: column;
  }

  .ripple-section {
    display: flex;
    flex-direction: column;
  }

  .right-section {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-md);
  }

  .reverb-panel,
  .io-panel {
    flex: 1;
  }

  .knob-row {
    display: flex;
    justify-content: space-around;
    align-items: flex-start;
    padding: var(--spacing-md) 0;
  }

  .knob-row.compact {
    padding: var(--spacing-sm) 0;
  }

  .slider-row {
    display: flex;
    gap: var(--spacing-lg);
    padding: var(--spacing-sm) var(--spacing-md);
  }

  .io-content {
    display: flex;
    justify-content: space-around;
    gap: var(--spacing-md);
  }

  .meter-group {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--spacing-sm);
  }

  .footer {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: var(--spacing-md);
    padding: var(--spacing-md);
    padding-top: 0;
  }
</style>

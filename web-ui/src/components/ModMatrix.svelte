<script lang="ts">
  import Select from './Select.svelte';
  import Knob from './Knob.svelte';
  import { createComboStore, createSliderStore } from '../stores/params';

  const modSources = ['None', 'LFO 1', 'LFO 2', 'LFO 3', 'LFO 4'];
  const modTargets = [
    'None',
    'Ripple Rate',
    'Ripple Multiply',
    'Ripple Amount',
    'Ripple Width',
    'Low Bypass',
    'High Bypass',
    'Ripple Mix',
    'Reverb Size',
    'Reverb Damping',
    'Reverb Mix'
  ];

  // Mod Slot 1
  const mod1Source = createComboStore('mod1_source', 1);
  const mod1Target = createComboStore('mod1_target', 0);
  const mod1Depth = createSliderStore('mod1_depth', 0.5);

  // Mod Slot 2
  const mod2Source = createComboStore('mod2_source', 2);
  const mod2Target = createComboStore('mod2_target', 0);
  const mod2Depth = createSliderStore('mod2_depth', 0.5);

  // Mod Slot 3
  const mod3Source = createComboStore('mod3_source', 3);
  const mod3Target = createComboStore('mod3_target', 0);
  const mod3Depth = createSliderStore('mod3_depth', 0.5);

  // Mod Slot 4
  const mod4Source = createComboStore('mod4_source', 4);
  const mod4Target = createComboStore('mod4_target', 0);
  const mod4Depth = createSliderStore('mod4_depth', 0.5);
</script>

<div class="mod-matrix panel">
  <div class="panel-header">
    <span class="panel-title">MODULATION MATRIX</span>
  </div>
  <div class="panel-content">
    <div class="matrix-grid">
      <!-- Header Row -->
      <div class="matrix-header">
        <span class="matrix-col-label">SLOT</span>
        <span class="matrix-col-label">SOURCE</span>
        <span class="matrix-col-label">TARGET</span>
        <span class="matrix-col-label">DEPTH</span>
      </div>

      <!-- Slot 1 -->
      <div class="matrix-row">
        <span class="slot-label">1</span>
        <Select store={mod1Source} options={modSources} />
        <Select store={mod1Target} options={modTargets} />
        <Knob store={mod1Depth} label="" size="small" bipolar />
      </div>

      <!-- Slot 2 -->
      <div class="matrix-row">
        <span class="slot-label">2</span>
        <Select store={mod2Source} options={modSources} />
        <Select store={mod2Target} options={modTargets} />
        <Knob store={mod2Depth} label="" size="small" bipolar />
      </div>

      <!-- Slot 3 -->
      <div class="matrix-row">
        <span class="slot-label">3</span>
        <Select store={mod3Source} options={modSources} />
        <Select store={mod3Target} options={modTargets} />
        <Knob store={mod3Depth} label="" size="small" bipolar />
      </div>

      <!-- Slot 4 -->
      <div class="matrix-row">
        <span class="slot-label">4</span>
        <Select store={mod4Source} options={modSources} />
        <Select store={mod4Target} options={modTargets} />
        <Knob store={mod4Depth} label="" size="small" bipolar />
      </div>
    </div>
  </div>
</div>

<style>
  .mod-matrix {
    height: 100%;
  }

  .matrix-grid {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-sm);
  }

  .matrix-header {
    display: grid;
    grid-template-columns: 40px 1fr 1fr 60px;
    gap: var(--spacing-sm);
    padding-bottom: var(--spacing-xs);
    border-bottom: 1px solid var(--border-subtle);
  }

  .matrix-col-label {
    font-size: 8px;
    font-weight: 600;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--text-muted);
    text-align: center;
  }

  .matrix-row {
    display: grid;
    grid-template-columns: 40px 1fr 1fr 60px;
    gap: var(--spacing-sm);
    align-items: center;
    padding: var(--spacing-xs) 0;
  }

  .matrix-row:not(:last-child) {
    border-bottom: 1px solid var(--border-subtle);
  }

  .slot-label {
    font-family: var(--font-display);
    font-size: 12px;
    font-weight: 600;
    color: var(--accent-cyan);
    text-align: center;
  }
</style>

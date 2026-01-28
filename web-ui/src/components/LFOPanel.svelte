<script lang="ts">
  import Knob from './Knob.svelte';
  import Select from './Select.svelte';
  import { createSliderStore, createComboStore, visualizerData } from '../stores/params';

  const lfoShapes = ['Sine', 'Triangle', 'Square', 'Saw Up', 'Saw Down', 'Random'];

  // LFO 1
  const lfo1Rate = createSliderStore('lfo1_rate', 0.5);
  const lfo1Shape = createComboStore('lfo1_shape', 0);
  const lfo1Phase = createSliderStore('lfo1_phase', 0);

  // LFO 2
  const lfo2Rate = createSliderStore('lfo2_rate', 0.6);
  const lfo2Shape = createComboStore('lfo2_shape', 0);
  const lfo2Phase = createSliderStore('lfo2_phase', 0.25);

  // LFO 3
  const lfo3Rate = createSliderStore('lfo3_rate', 0.7);
  const lfo3Shape = createComboStore('lfo3_shape', 0);
  const lfo3Phase = createSliderStore('lfo3_phase', 0.5);

  // LFO 4
  const lfo4Rate = createSliderStore('lfo4_rate', 0.8);
  const lfo4Shape = createComboStore('lfo4_shape', 0);
  const lfo4Phase = createSliderStore('lfo4_phase', 0.75);

  $: lfoValues = $visualizerData.lfoValues;
</script>

<div class="lfo-panel panel">
  <div class="panel-header">
    <span class="panel-title">LFO MODULATORS</span>
  </div>
  <div class="panel-content">
    <div class="lfo-grid">
      <!-- LFO 1 -->
      <div class="lfo-unit">
        <div class="lfo-header">
          <span class="lfo-label">LFO 1</span>
          <div class="lfo-indicator" style="--lfo-value: {lfoValues[0]}"></div>
        </div>
        <div class="lfo-controls">
          <Knob label="RATE" store={lfo1Rate} min={0.01} max={20} unit="Hz" skew={0.3} size="small" />
          <Select label="SHAPE" store={lfo1Shape} options={lfoShapes} />
          <Knob label="PHASE" store={lfo1Phase} size="small" />
        </div>
      </div>

      <!-- LFO 2 -->
      <div class="lfo-unit">
        <div class="lfo-header">
          <span class="lfo-label">LFO 2</span>
          <div class="lfo-indicator" style="--lfo-value: {lfoValues[1]}"></div>
        </div>
        <div class="lfo-controls">
          <Knob label="RATE" store={lfo2Rate} min={0.01} max={20} unit="Hz" skew={0.3} size="small" />
          <Select label="SHAPE" store={lfo2Shape} options={lfoShapes} />
          <Knob label="PHASE" store={lfo2Phase} size="small" />
        </div>
      </div>

      <!-- LFO 3 -->
      <div class="lfo-unit">
        <div class="lfo-header">
          <span class="lfo-label">LFO 3</span>
          <div class="lfo-indicator" style="--lfo-value: {lfoValues[2]}"></div>
        </div>
        <div class="lfo-controls">
          <Knob label="RATE" store={lfo3Rate} min={0.01} max={20} unit="Hz" skew={0.3} size="small" />
          <Select label="SHAPE" store={lfo3Shape} options={lfoShapes} />
          <Knob label="PHASE" store={lfo3Phase} size="small" />
        </div>
      </div>

      <!-- LFO 4 -->
      <div class="lfo-unit">
        <div class="lfo-header">
          <span class="lfo-label">LFO 4</span>
          <div class="lfo-indicator" style="--lfo-value: {lfoValues[3]}"></div>
        </div>
        <div class="lfo-controls">
          <Knob label="RATE" store={lfo4Rate} min={0.01} max={20} unit="Hz" skew={0.3} size="small" />
          <Select label="SHAPE" store={lfo4Shape} options={lfoShapes} />
          <Knob label="PHASE" store={lfo4Phase} size="small" />
        </div>
      </div>
    </div>
  </div>
</div>

<style>
  .lfo-panel {
    height: 100%;
  }

  .lfo-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: var(--spacing-md);
  }

  .lfo-unit {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-sm);
  }

  .lfo-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding-bottom: var(--spacing-xs);
    border-bottom: 1px solid var(--border-subtle);
  }

  .lfo-label {
    font-family: var(--font-display);
    font-size: 10px;
    font-weight: 600;
    letter-spacing: 0.1em;
    color: var(--text-accent);
  }

  .lfo-indicator {
    width: 20px;
    height: 4px;
    background: var(--bg-tertiary);
    border-radius: 2px;
    overflow: hidden;
    position: relative;
  }

  .lfo-indicator::after {
    content: '';
    position: absolute;
    top: 0;
    left: 0;
    height: 100%;
    width: calc(var(--lfo-value, 0.5) * 100%);
    background: linear-gradient(90deg, var(--accent-cyan-muted), var(--accent-cyan));
    transition: width 50ms ease-out;
  }

  .lfo-controls {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--spacing-sm);
  }
</style>

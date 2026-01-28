<script lang="ts">
  import type { SliderStore } from '../stores/params';

  export let label: string;
  export let store: SliderStore;
  export let min: number = 0;
  export let max: number = 1;
  export let unit: string = '';

  $: value = $store;
  $: displayValue = getDisplayValue(value);
  $: fillPercent = value * 100;

  function getDisplayValue(normalizedValue: number): string {
    const actualValue = min + normalizedValue * (max - min);
    if (actualValue >= 1000) {
      return (actualValue / 1000).toFixed(1) + 'k';
    }
    return Math.round(actualValue).toString();
  }

  function handleInput(e: Event) {
    const target = e.target as HTMLInputElement;
    store.set(parseFloat(target.value));
  }

  function handleMouseDown() {
    store.onDragStart();
  }

  function handleMouseUp() {
    store.onDragEnd();
  }
</script>

<div class="slider-container">
  <div class="slider-header">
    <span class="slider-label">{label}</span>
    <span class="slider-value">{displayValue}{unit ? ' ' + unit : ''}</span>
  </div>
  <div class="slider-track">
    <div class="slider-fill" style="width: {fillPercent}%"></div>
    <input
      type="range"
      min="0"
      max="1"
      step="0.001"
      value={value}
      on:input={handleInput}
      on:mousedown={handleMouseDown}
      on:mouseup={handleMouseUp}
      class="slider-input"
    />
  </div>
</div>

<style>
  .slider-container {
    flex: 1;
    min-width: 100px;
  }

  .slider-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: var(--spacing-xs);
  }

  .slider-label {
    font-size: 9px;
    font-weight: 500;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--text-secondary);
  }

  .slider-value {
    font-family: var(--font-display);
    font-size: 10px;
    font-weight: 500;
    color: var(--text-primary);
  }

  .slider-track {
    position: relative;
    height: 6px;
    background: var(--slider-track);
    border-radius: 3px;
    overflow: hidden;
  }

  .slider-fill {
    position: absolute;
    top: 0;
    left: 0;
    height: 100%;
    background: var(--slider-fill);
    border-radius: 3px;
    pointer-events: none;
    transition: width 50ms ease-out;
  }

  .slider-input {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    opacity: 0;
    cursor: pointer;
    margin: 0;
  }

  .slider-track::after {
    content: '';
    position: absolute;
    right: 0;
    top: 50%;
    width: 12px;
    height: 12px;
    background: var(--text-primary);
    border-radius: 50%;
    transform: translate(50%, -50%);
    box-shadow: 0 0 8px var(--glow-cyan);
    pointer-events: none;
    left: calc(var(--value, 0) * 100%);
  }
</style>

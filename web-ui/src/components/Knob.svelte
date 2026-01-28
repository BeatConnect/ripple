<script lang="ts">
  import type { SliderStore } from '../stores/params';

  export let label: string;
  export let store: SliderStore;
  export let min: number = 0;
  export let max: number = 1;
  export let unit: string = '';
  export let skew: number = 1;
  export let size: 'small' | 'medium' | 'large' = 'medium';
  export let bipolar: boolean = false;

  let isDragging = false;
  let startY = 0;
  let startValue = 0;

  $: value = $store;
  $: displayValue = getDisplayValue(value);
  $: rotation = getRotation(value);

  function getDisplayValue(normalizedValue: number): string {
    // Apply inverse skew for display
    const skewedValue = Math.pow(normalizedValue, skew);
    const actualValue = min + skewedValue * (max - min);

    if (bipolar) {
      const centered = (normalizedValue - 0.5) * 2 * (max - min) / 2;
      if (Math.abs(centered) < 0.1) return '0';
      return centered.toFixed(1);
    }

    if (actualValue >= 1000) {
      return (actualValue / 1000).toFixed(1) + 'k';
    }
    if (actualValue < 1) {
      return actualValue.toFixed(2);
    }
    return actualValue.toFixed(1);
  }

  function getRotation(normalizedValue: number): number {
    // -135 to +135 degrees
    return -135 + normalizedValue * 270;
  }

  function handleMouseDown(e: MouseEvent) {
    isDragging = true;
    startY = e.clientY;
    startValue = value;
    store.onDragStart();
    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);
  }

  function handleMouseMove(e: MouseEvent) {
    if (!isDragging) return;

    const deltaY = startY - e.clientY;
    const sensitivity = e.shiftKey ? 0.001 : 0.005;
    const newValue = Math.max(0, Math.min(1, startValue + deltaY * sensitivity));
    store.set(newValue);
  }

  function handleMouseUp() {
    isDragging = false;
    store.onDragEnd();
    window.removeEventListener('mousemove', handleMouseMove);
    window.removeEventListener('mouseup', handleMouseUp);
  }

  function handleDoubleClick() {
    store.set(bipolar ? 0.5 : 0.5);
  }
</script>

<div class="knob-container {size}" class:dragging={isDragging}>
  <div
    class="knob"
    on:mousedown={handleMouseDown}
    on:dblclick={handleDoubleClick}
    role="slider"
    aria-valuenow={value}
    tabindex="0"
  >
    <svg viewBox="0 0 100 100" class="knob-svg">
      <!-- Track background -->
      <circle
        cx="50"
        cy="50"
        r="40"
        fill="none"
        stroke="var(--knob-track)"
        stroke-width="4"
        stroke-dasharray="188.5"
        stroke-dashoffset="47"
        stroke-linecap="round"
        transform="rotate(135 50 50)"
      />

      <!-- Active track -->
      <circle
        cx="50"
        cy="50"
        r="40"
        fill="none"
        stroke="var(--accent-cyan)"
        stroke-width="4"
        stroke-dasharray="{value * 188.5} 188.5"
        stroke-dashoffset="47"
        stroke-linecap="round"
        transform="rotate(135 50 50)"
        class="track-fill"
      />

      <!-- Knob body -->
      <circle
        cx="50"
        cy="50"
        r="32"
        fill="var(--bg-tertiary)"
        stroke="var(--border-subtle)"
        stroke-width="1"
      />

      <!-- Indicator line -->
      <line
        x1="50"
        y1="22"
        x2="50"
        y2="35"
        stroke="var(--text-primary)"
        stroke-width="3"
        stroke-linecap="round"
        transform="rotate({rotation} 50 50)"
        class="indicator"
      />
    </svg>
  </div>

  <div class="knob-label">{label}</div>
  <div class="knob-value">{displayValue}{unit ? ' ' + unit : ''}</div>
</div>

<style>
  .knob-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--spacing-xs);
  }

  .knob-container.small .knob { width: 48px; height: 48px; }
  .knob-container.medium .knob { width: 64px; height: 64px; }
  .knob-container.large .knob { width: 96px; height: 96px; }

  .knob {
    cursor: grab;
    touch-action: none;
  }

  .knob:active {
    cursor: grabbing;
  }

  .knob-svg {
    width: 100%;
    height: 100%;
    filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.3));
  }

  .track-fill {
    transition: stroke-dasharray 50ms ease-out;
    filter: drop-shadow(0 0 6px var(--glow-cyan));
  }

  .indicator {
    transition: transform 50ms ease-out;
  }

  .dragging .indicator {
    stroke: var(--accent-cyan);
  }

  .knob-label {
    font-size: 9px;
    font-weight: 500;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--text-secondary);
  }

  .knob-value {
    font-family: var(--font-display);
    font-size: 10px;
    font-weight: 500;
    color: var(--text-primary);
  }
</style>

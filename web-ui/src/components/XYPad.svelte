<script lang="ts">
  import { createEventDispatcher } from 'svelte';

  export let x: number = 0.5;
  export let y: number = 0.5;

  const dispatch = createEventDispatcher();

  // Zone presets - values for [rate, intensity, width]
  const zones = {
    drift:   { rate: 0.15, intensity: 0.25, width: 0.7 },
    shimmer: { rate: 0.85, intensity: 0.3, width: 0.25 },
    pulse:   { rate: 0.2, intensity: 0.85, width: 0.5 },
    chaos:   { rate: 0.9, intensity: 1.0, width: 0.85 },
  };

  let isDragging = false;
  let padElement: HTMLDivElement;

  // Bilinear interpolation
  function interpolate(x: number, y: number) {
    const bl = zones.drift, br = zones.shimmer;
    const tl = zones.pulse, tr = zones.chaos;

    const bottom = {
      rate: bl.rate + (br.rate - bl.rate) * x,
      intensity: bl.intensity + (br.intensity - bl.intensity) * x,
      width: bl.width + (br.width - bl.width) * x,
    };
    const top = {
      rate: tl.rate + (tr.rate - tl.rate) * x,
      intensity: tl.intensity + (tr.intensity - tl.intensity) * x,
      width: tl.width + (tr.width - tl.width) * x,
    };

    return {
      rate: bottom.rate + (top.rate - bottom.rate) * y,
      intensity: bottom.intensity + (top.intensity - bottom.intensity) * y,
      width: bottom.width + (top.width - bottom.width) * y,
    };
  }

  function updatePosition(clientX: number, clientY: number) {
    if (!padElement) return;
    const rect = padElement.getBoundingClientRect();
    const newX = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width));
    const newY = Math.max(0, Math.min(1, 1 - (clientY - rect.top) / rect.height));

    x = newX;
    y = newY;

    const values = interpolate(x, y);
    dispatch('change', { x, y, ...values });
  }

  function handleMouseDown(e: MouseEvent) {
    isDragging = true;
    updatePosition(e.clientX, e.clientY);
    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);
  }

  function handleMouseMove(e: MouseEvent) {
    if (!isDragging) return;
    updatePosition(e.clientX, e.clientY);
  }

  function handleMouseUp() {
    isDragging = false;
    window.removeEventListener('mousemove', handleMouseMove);
    window.removeEventListener('mouseup', handleMouseUp);
  }

  $: zoneName = x < 0.5
    ? (y < 0.5 ? 'DRIFT' : 'PULSE')
    : (y < 0.5 ? 'SHIMMER' : 'CHAOS');
</script>

<div class="xy-wrapper">
  <div
    class="xy-pad"
    bind:this={padElement}
    on:mousedown={handleMouseDown}
    class:dragging={isDragging}
  >
    <!-- Zone labels -->
    <span class="zone top-left" class:active={x < 0.5 && y >= 0.5}>PULSE</span>
    <span class="zone top-right" class:active={x >= 0.5 && y >= 0.5}>CHAOS</span>
    <span class="zone bottom-left" class:active={x < 0.5 && y < 0.5}>DRIFT</span>
    <span class="zone bottom-right" class:active={x >= 0.5 && y < 0.5}>SHIMMER</span>

    <!-- Crosshair -->
    <div class="crosshair-h" style="bottom: {y * 100}%"></div>
    <div class="crosshair-v" style="left: {x * 100}%"></div>

    <!-- Cursor -->
    <div
      class="cursor"
      class:active={isDragging}
      style="left: {x * 100}%; bottom: {y * 100}%"
    ></div>
  </div>

  <div class="zone-display">{zoneName}</div>
</div>

<style>
  .xy-wrapper {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 16px;
  }

  .xy-pad {
    width: 320px;
    height: 320px;
    position: relative;
    background: rgba(10, 10, 18, 0.6);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 8px;
    cursor: crosshair;
    backdrop-filter: blur(10px);
    overflow: hidden;
  }

  .xy-pad::before {
    content: '';
    position: absolute;
    inset: 0;
    background:
      linear-gradient(135deg, rgba(100, 180, 180, 0.05) 0%, transparent 50%),
      linear-gradient(225deg, rgba(150, 100, 180, 0.05) 0%, transparent 50%),
      linear-gradient(315deg, rgba(180, 100, 130, 0.05) 0%, transparent 50%),
      linear-gradient(45deg, rgba(180, 130, 80, 0.05) 0%, transparent 50%);
    pointer-events: none;
  }

  .zone {
    position: absolute;
    font-family: 'SF Pro Display', 'Segoe UI', system-ui, sans-serif;
    font-size: 11px;
    font-weight: 500;
    letter-spacing: 0.2em;
    color: rgba(255, 255, 255, 0.2);
    padding: 12px;
    transition: all 0.3s ease;
    user-select: none;
  }

  .zone.active {
    color: rgba(255, 255, 255, 0.7);
  }

  .top-left { top: 0; left: 0; }
  .top-right { top: 0; right: 0; }
  .bottom-left { bottom: 0; left: 0; }
  .bottom-right { bottom: 0; right: 0; }

  .crosshair-h, .crosshair-v {
    position: absolute;
    background: rgba(255, 255, 255, 0.1);
    pointer-events: none;
  }

  .crosshair-h {
    left: 0;
    right: 0;
    height: 1px;
  }

  .crosshair-v {
    top: 0;
    bottom: 0;
    width: 1px;
  }

  .cursor {
    position: absolute;
    width: 20px;
    height: 20px;
    transform: translate(-50%, 50%);
    border-radius: 50%;
    background: rgba(255, 255, 255, 0.9);
    box-shadow:
      0 0 20px rgba(255, 255, 255, 0.4),
      0 0 40px rgba(100, 180, 180, 0.3);
    pointer-events: none;
    transition: transform 0.1s ease, box-shadow 0.2s ease;
  }

  .cursor.active {
    transform: translate(-50%, 50%) scale(1.2);
    box-shadow:
      0 0 30px rgba(255, 255, 255, 0.6),
      0 0 60px rgba(100, 180, 180, 0.5);
  }

  .xy-pad.dragging {
    border-color: rgba(255, 255, 255, 0.15);
  }

  .zone-display {
    font-family: 'SF Pro Display', 'Segoe UI', system-ui, sans-serif;
    font-size: 14px;
    font-weight: 600;
    letter-spacing: 0.3em;
    color: rgba(255, 255, 255, 0.6);
  }
</style>

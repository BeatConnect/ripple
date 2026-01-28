<script lang="ts">
  import { onMount } from 'svelte';

  export let bands: number[] = new Array(16).fill(0.5);
  export let lfoValues: number[] = [0.5, 0.5, 0.5, 0.5];

  let canvas: HTMLCanvasElement;
  let ctx: CanvasRenderingContext2D | null;
  let animationId: number;
  let time = 0;

  const bandFrequencies = [
    '31', '50', '80', '125', '200', '315', '500', '800',
    '1.2k', '2k', '3.2k', '5k', '8k', '10k', '12k', '16k'
  ];

  onMount(() => {
    ctx = canvas.getContext('2d');
    if (!ctx) return;

    function draw() {
      if (!ctx) return;

      const width = canvas.width;
      const height = canvas.height;
      const centerX = width / 2;
      const centerY = height / 2;
      const maxRadius = Math.min(width, height) * 0.4;

      // Clear with fade effect
      ctx.fillStyle = 'rgba(10, 10, 18, 0.1)';
      ctx.fillRect(0, 0, width, height);

      // Draw circular ripple bands
      const numBands = bands.length;
      const angleStep = (Math.PI * 2) / numBands;

      // Draw outer ring
      ctx.beginPath();
      ctx.arc(centerX, centerY, maxRadius + 5, 0, Math.PI * 2);
      ctx.strokeStyle = 'rgba(94, 173, 176, 0.2)';
      ctx.lineWidth = 1;
      ctx.stroke();

      // Draw bands as radial bars
      for (let i = 0; i < numBands; i++) {
        const angle = i * angleStep - Math.PI / 2;
        const magnitude = bands[i];
        const barLength = magnitude * maxRadius * 0.8;

        // Inner radius
        const innerRadius = maxRadius * 0.2;
        const outerRadius = innerRadius + barLength;

        // Calculate positions
        const x1 = centerX + Math.cos(angle) * innerRadius;
        const y1 = centerY + Math.sin(angle) * innerRadius;
        const x2 = centerX + Math.cos(angle) * outerRadius;
        const y2 = centerY + Math.sin(angle) * outerRadius;

        // Create gradient
        const gradient = ctx.createLinearGradient(x1, y1, x2, y2);
        gradient.addColorStop(0, 'rgba(94, 173, 176, 0.3)');
        gradient.addColorStop(0.5, 'rgba(94, 173, 176, 0.8)');
        gradient.addColorStop(1, 'rgba(168, 85, 168, 0.8)');

        // Draw bar
        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.lineTo(x2, y2);
        ctx.strokeStyle = gradient;
        ctx.lineWidth = 8;
        ctx.lineCap = 'round';
        ctx.stroke();

        // Draw glow
        ctx.shadowColor = 'rgba(94, 173, 176, 0.5)';
        ctx.shadowBlur = 10;
      }

      ctx.shadowBlur = 0;

      // Draw LFO rings
      const avgLfo = lfoValues.reduce((a, b) => a + b, 0) / lfoValues.length;
      const pulseRadius = maxRadius * 0.15 + avgLfo * maxRadius * 0.1;

      ctx.beginPath();
      ctx.arc(centerX, centerY, pulseRadius, 0, Math.PI * 2);
      ctx.strokeStyle = `rgba(124, 92, 224, ${0.3 + avgLfo * 0.4})`;
      ctx.lineWidth = 2;
      ctx.stroke();

      // Draw inner pulsing circle
      ctx.beginPath();
      ctx.arc(centerX, centerY, pulseRadius * 0.6, 0, Math.PI * 2);
      const innerGradient = ctx.createRadialGradient(
        centerX, centerY, 0,
        centerX, centerY, pulseRadius * 0.6
      );
      innerGradient.addColorStop(0, `rgba(94, 173, 176, ${0.3 + avgLfo * 0.5})`);
      innerGradient.addColorStop(1, 'rgba(94, 173, 176, 0)');
      ctx.fillStyle = innerGradient;
      ctx.fill();

      // Draw animated ripple waves
      for (let w = 0; w < 3; w++) {
        const wavePhase = (time * 0.5 + w * 0.33) % 1;
        const waveRadius = maxRadius * 0.3 + wavePhase * maxRadius * 0.7;
        const waveAlpha = (1 - wavePhase) * 0.3;

        ctx.beginPath();
        ctx.arc(centerX, centerY, waveRadius, 0, Math.PI * 2);
        ctx.strokeStyle = `rgba(168, 85, 168, ${waveAlpha})`;
        ctx.lineWidth = 1;
        ctx.stroke();
      }

      time += 0.016;
      animationId = requestAnimationFrame(draw);
    }

    // Set canvas size
    function resize() {
      const rect = canvas.getBoundingClientRect();
      canvas.width = rect.width * window.devicePixelRatio;
      canvas.height = rect.height * window.devicePixelRatio;
      ctx?.scale(window.devicePixelRatio, window.devicePixelRatio);
    }

    resize();
    window.addEventListener('resize', resize);
    draw();

    return () => {
      cancelAnimationFrame(animationId);
      window.removeEventListener('resize', resize);
    };
  });
</script>

<div class="visualizer">
  <canvas bind:this={canvas}></canvas>

  <!-- Frequency labels -->
  <div class="freq-labels">
    {#each bandFrequencies as freq, i}
      <span
        class="freq-label"
        style="
          --angle: {(i / bandFrequencies.length) * 360 - 90}deg;
          --intensity: {bands[i]};
        "
      >
        {freq}
      </span>
    {/each}
  </div>
</div>

<style>
  .visualizer {
    position: relative;
    width: 100%;
    height: 100%;
    min-height: 200px;
  }

  canvas {
    position: absolute;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
  }

  .freq-labels {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 0;
    height: 0;
  }

  .freq-label {
    position: absolute;
    font-size: 8px;
    font-weight: 500;
    color: var(--text-muted);
    transform: translate(-50%, -50%)
      rotate(var(--angle))
      translateY(-110px)
      rotate(calc(-1 * var(--angle)));
    opacity: calc(0.4 + var(--intensity) * 0.6);
    transition: opacity 100ms ease-out;
  }
</style>

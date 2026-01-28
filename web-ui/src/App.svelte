<script lang="ts">
  import { onMount } from 'svelte';
  import { addCustomEventListener } from './lib/juce-bridge';

  let canvas: HTMLCanvasElement;
  let ctx: CanvasRenderingContext2D;
  let spectrum: number[] = new Array(256).fill(0);
  let smoothSpectrum: number[] = new Array(256).fill(0);
  let frozen: number[] = new Array(256).fill(0);
  let animationId: number;
  let time = 0;

  // Parameters
  let freeze = 0.3;
  let sustain = 0.4;
  let diffuse = 0.2;
  let shift = 0.5;
  let tilt = 0.55;
  let feedback = 0.15;

  // LFO state
  let lfoEnabled = false;
  let lfoPanelOpen = false;
  let lfoShape: 'circle' | 'figure8' | 'wave' | 'random' = 'circle';
  let lfoRate = 0.3;
  let lfoDepth = 0.5;
  let lfoX = 0;
  let lfoY = 0;

  let lfoTargets = {
    freeze: { enabled: false, invert: false },
    sustain: { enabled: false, invert: false },
    diffuse: { enabled: false, invert: false },
    shift: { enabled: true, invert: false },
    tilt: { enabled: true, invert: false },
    feedback: { enabled: false, invert: false }
  };

  const presets = [
    { name: 'INIT', freeze: 0.3, sustain: 0.4, diffuse: 0.2, shift: 0.5, tilt: 0.55, feedback: 0.15, lfo: false, lfoShape: 'circle', lfoRate: 0.3, lfoDepth: 0.5, targets: { shift: true, tilt: true } },
    { name: 'FROZEN PAD', freeze: 0.8, sustain: 0.6, diffuse: 0.2, shift: 0.5, tilt: 0.6, feedback: 0.3, lfo: true, lfoShape: 'wave', lfoRate: 0.15, lfoDepth: 0.4, targets: { freeze: true, tilt: true } },
    { name: 'SHIMMER', freeze: 0.3, sustain: 0.7, diffuse: 0.5, shift: 0.65, tilt: 0.7, feedback: 0.4, lfo: true, lfoShape: 'circle', lfoRate: 0.5, lfoDepth: 0.6, targets: { shift: true, diffuse: true } },
    { name: 'DARK DRONE', freeze: 0.6, sustain: 0.9, diffuse: 0.3, shift: 0.35, tilt: 0.2, feedback: 0.5, lfo: true, lfoShape: 'figure8', lfoRate: 0.1, lfoDepth: 0.7, targets: { tilt: true, feedback: true } },
    { name: 'CRYSTALLINE', freeze: 0.4, sustain: 0.3, diffuse: 0.8, shift: 0.55, tilt: 0.8, feedback: 0.2, lfo: true, lfoShape: 'random', lfoRate: 0.4, lfoDepth: 0.5, targets: { diffuse: true, shift: true } },
    { name: 'EVOLVING', freeze: 0.5, sustain: 0.5, diffuse: 0.4, shift: 0.5, tilt: 0.5, feedback: 0.7, lfo: true, lfoShape: 'wave', lfoRate: 0.25, lfoDepth: 0.8, targets: { freeze: true, sustain: true, shift: true, tilt: true } },
    { name: 'GHOST', freeze: 0.7, sustain: 0.8, diffuse: 0.6, shift: 0.45, tilt: 0.4, feedback: 0.6, lfo: true, lfoShape: 'figure8', lfoRate: 0.2, lfoDepth: 0.6, targets: { sustain: true, diffuse: true, feedback: true } },
    { name: 'BRIGHT WASH', freeze: 0.5, sustain: 0.7, diffuse: 0.4, shift: 0.7, tilt: 0.85, feedback: 0.3, lfo: true, lfoShape: 'circle', lfoRate: 0.35, lfoDepth: 0.5, targets: { shift: true, tilt: true } },
  ];
  let selectedPreset = 'INIT';
  let presetMenuOpen = false;

  function loadPreset(name: string) {
    const preset = presets.find(p => p.name === name);
    if (preset) {
      freeze = preset.freeze;
      sustain = preset.sustain;
      diffuse = preset.diffuse;
      shift = preset.shift;
      tilt = preset.tilt;
      feedback = preset.feedback;

      // Apply LFO settings
      lfoEnabled = preset.lfo;
      lfoShape = preset.lfoShape as typeof lfoShape;
      lfoRate = preset.lfoRate;
      lfoDepth = preset.lfoDepth;

      // Reset all targets then enable specified ones
      Object.keys(lfoTargets).forEach(k => {
        const key = k as keyof typeof lfoTargets;
        lfoTargets[key].enabled = preset.targets[key] || false;
        lfoTargets[key].invert = false;
      });

      selectedPreset = name;
      presetMenuOpen = false;
    }
  }

  addCustomEventListener('spectrumData', (data: any) => {
    if (data.spectrum) spectrum = data.spectrum;
    if (data.frozen) frozen = data.frozen;
  });

  function setParam(name: string, value: number) {
    const w = window as any;
    if (w.__JUCE__?.backend?.emitEvent) {
      w.__JUCE__.backend.emitEvent(`${name}_change`, { value });
    }
  }

  function resizeCanvas() {
    const dpr = window.devicePixelRatio || 1;
    canvas.width = window.innerWidth * dpr;
    canvas.height = window.innerHeight * dpr;
    canvas.style.width = window.innerWidth + 'px';
    canvas.style.height = window.innerHeight + 'px';
    ctx.scale(dpr, dpr);
  }

  function isoToScreen(x: number, y: number, z: number, origin: {x: number, y: number}) {
    const isoX = (x - y) * 0.866;
    const isoY = (x + y) * 0.5 - z;
    return { x: origin.x + isoX, y: origin.y + isoY };
  }

  function drawIsoColumn(
    gridX: number, gridY: number, height: number,
    cellSize: number, origin: {x: number, y: number},
    intensity: number, frozenAmt: number, lfoInfluence: number
  ) {
    const h = Math.max(4, height);
    const base2 = isoToScreen((gridX + 1) * cellSize, gridY * cellSize, 0, origin);
    const base3 = isoToScreen((gridX + 1) * cellSize, (gridY + 1) * cellSize, 0, origin);
    const base4 = isoToScreen(gridX * cellSize, (gridY + 1) * cellSize, 0, origin);
    const top1 = isoToScreen(gridX * cellSize, gridY * cellSize, h, origin);
    const top2 = isoToScreen((gridX + 1) * cellSize, gridY * cellSize, h, origin);
    const top3 = isoToScreen((gridX + 1) * cellSize, (gridY + 1) * cellSize, h, origin);
    const top4 = isoToScreen(gridX * cellSize, (gridY + 1) * cellSize, h, origin);

    const hue = 200 + frozenAmt * 30 - intensity * 10 + lfoInfluence * 40;
    const sat = 40 + intensity * 45 + frozenAmt * 15;

    if (intensity > 0.15 || frozenAmt > 0.2) {
      const cx = (top1.x + top3.x) / 2;
      const cy = (top1.y + top3.y) / 2;
      const grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, cellSize * 1.5 + h * 0.3);
      grad.addColorStop(0, `hsla(${hue}, 85%, 65%, ${(intensity + frozenAmt * 0.5) * 0.4})`);
      grad.addColorStop(1, 'transparent');
      ctx.fillStyle = grad;
      ctx.beginPath();
      ctx.arc(cx, cy, cellSize * 1.5 + h * 0.3, 0, Math.PI * 2);
      ctx.fill();
    }

    ctx.beginPath();
    ctx.moveTo(base2.x, base2.y);
    ctx.lineTo(base3.x, base3.y);
    ctx.lineTo(top3.x, top3.y);
    ctx.lineTo(top2.x, top2.y);
    ctx.closePath();
    ctx.fillStyle = `hsla(${hue + 10}, ${sat}%, ${10 + intensity * 22}%, 0.95)`;
    ctx.fill();

    ctx.beginPath();
    ctx.moveTo(base3.x, base3.y);
    ctx.lineTo(base4.x, base4.y);
    ctx.lineTo(top4.x, top4.y);
    ctx.lineTo(top3.x, top3.y);
    ctx.closePath();
    ctx.fillStyle = `hsla(${hue}, ${sat}%, ${16 + intensity * 30}%, 0.95)`;
    ctx.fill();

    ctx.beginPath();
    ctx.moveTo(top1.x, top1.y);
    ctx.lineTo(top2.x, top2.y);
    ctx.lineTo(top3.x, top3.y);
    ctx.lineTo(top4.x, top4.y);
    ctx.closePath();
    ctx.fillStyle = `hsla(${hue - 5}, ${sat + 10}%, ${32 + intensity * 42 + frozenAmt * 20}%, 0.98)`;
    ctx.fill();
  }

  function drawAurora(w: number, h: number) {
    const t = time * 0.2;
    // Subtle aurora bands - mostly blue/cyan with hints of green and purple
    for (let band = 0; band < 4; band++) {
      const yBase = h * (0.02 + band * 0.12);
      const amplitude = h * (0.08 + band * 0.02);
      // Blue-dominant with subtle color variation
      const hue = 180 + band * 25 + Math.sin(t * 0.2 + band) * 15;

      ctx.beginPath();
      ctx.moveTo(0, 0);
      for (let x = 0; x <= w; x += 10) {
        const y = yBase + Math.sin(x * 0.003 + t + band * 0.6) * amplitude +
                  Math.sin(x * 0.006 + t * 0.8) * amplitude * 0.3;
        ctx.lineTo(x, y);
      }
      ctx.lineTo(w, 0);
      ctx.closePath();

      const grad = ctx.createLinearGradient(0, 0, 0, yBase + amplitude * 2.5);
      const alpha = 0.08 + freeze * 0.06;
      grad.addColorStop(0, 'transparent');
      grad.addColorStop(0.4, `hsla(${hue}, 65%, 50%, ${alpha * 0.6})`);
      grad.addColorStop(0.7, `hsla(${hue}, 60%, 40%, ${alpha})`);
      grad.addColorStop(1, 'transparent');
      ctx.fillStyle = grad;
      ctx.fill();
    }
  }

  function updateLFO() {
    if (!lfoEnabled) { lfoX = lfoY = 0; return; }
    const t = time * lfoRate * 3;
    switch (lfoShape) {
      case 'circle': lfoX = Math.cos(t) * lfoDepth; lfoY = Math.sin(t) * lfoDepth; break;
      case 'figure8': lfoX = Math.sin(t) * lfoDepth; lfoY = Math.sin(t * 2) * lfoDepth * 0.5; break;
      case 'wave': lfoX = Math.sin(t) * lfoDepth; lfoY = Math.sin(t * 1.5 + 1) * lfoDepth; break;
      case 'random': lfoX = Math.sin(t * 0.7) * Math.cos(t * 1.3) * lfoDepth; lfoY = Math.cos(t * 0.9) * Math.sin(t * 1.1) * lfoDepth; break;
    }
  }

  function applyLFOModulation() {
    if (!lfoEnabled) return;
    if (lfoTargets.freeze.enabled) setParam('freeze', Math.max(0, Math.min(1, freeze + (lfoTargets.freeze.invert ? -lfoY : lfoY) * 0.3)));
    if (lfoTargets.sustain.enabled) setParam('smear', Math.max(0, Math.min(1, sustain + (lfoTargets.sustain.invert ? -lfoX : lfoX) * 0.3)));
    if (lfoTargets.diffuse.enabled) setParam('scatter', Math.max(0, Math.min(1, diffuse + (lfoTargets.diffuse.invert ? -lfoY : lfoY) * 0.3)));
    if (lfoTargets.shift.enabled) setParam('shift', Math.max(0, Math.min(1, 0.5 + (lfoTargets.shift.invert ? -lfoX : lfoX) * 0.4)));
    if (lfoTargets.tilt.enabled) setParam('tilt', Math.max(0, Math.min(1, 0.5 + (lfoTargets.tilt.invert ? -lfoY : lfoY) * 0.4)));
    if (lfoTargets.feedback.enabled) setParam('feedback', Math.max(0, Math.min(1, feedback + (lfoTargets.feedback.invert ? -lfoX : lfoX) * 0.3)));
  }

  function render() {
    animationId = requestAnimationFrame(render);
    time += 0.016;
    const w = window.innerWidth, h = window.innerHeight;
    updateLFO();

    const bgGrad = ctx.createLinearGradient(0, 0, 0, h);
    bgGrad.addColorStop(0, '#010204');
    bgGrad.addColorStop(0.5, '#040810');
    bgGrad.addColorStop(1, '#030608');
    ctx.fillStyle = bgGrad;
    ctx.fillRect(0, 0, w, h);

    drawAurora(w, h);

    for (let i = 0; i < spectrum.length; i++) {
      const target = spectrum[i];
      smoothSpectrum[i] = target > smoothSpectrum[i] ? smoothSpectrum[i] * 0.5 + target * 0.5 : smoothSpectrum[i] * 0.92 + target * 0.08;
    }

    if (!lfoEnabled || !lfoTargets.freeze.enabled) setParam('freeze', freeze);
    if (!lfoEnabled || !lfoTargets.sustain.enabled) setParam('smear', sustain);
    if (!lfoEnabled || !lfoTargets.diffuse.enabled) setParam('scatter', diffuse);
    if (!lfoEnabled || !lfoTargets.shift.enabled) setParam('shift', shift);
    if (!lfoEnabled || !lfoTargets.tilt.enabled) setParam('tilt', tilt);
    if (!lfoEnabled || !lfoTargets.feedback.enabled) setParam('feedback', feedback);
    if (lfoEnabled) applyLFOModulation();

    const gridCols = 32, gridRows = 5, cellSize = 14, maxHeight = 130;
    const gridWidthScreen = gridCols * cellSize * 0.866 + gridRows * cellSize * 0.866;
    const gridHeightScreen = (gridCols + gridRows) * cellSize * 0.5;
    // Account for header (~100px) and footer (~130px) when centering
    const headerHeight = 100, footerHeight = 130;
    const availableHeight = h - headerHeight - footerHeight;
    const centerY = headerHeight + availableHeight / 2;
    const origin = { x: w / 2 - gridWidthScreen / 2 + gridRows * cellSize * 0.866, y: centerY - gridHeightScreen / 2 + maxHeight / 2 - 15 };

    ctx.strokeStyle = 'rgba(80, 140, 200, 0.04)';
    ctx.lineWidth = 1;
    for (let y = 0; y <= gridRows; y++) {
      const start = isoToScreen(0, y * cellSize, 0, origin);
      const end = isoToScreen(gridCols * cellSize, y * cellSize, 0, origin);
      ctx.beginPath(); ctx.moveTo(start.x, start.y); ctx.lineTo(end.x, end.y); ctx.stroke();
    }
    for (let x = 0; x <= gridCols; x++) {
      const start = isoToScreen(x * cellSize, 0, 0, origin);
      const end = isoToScreen(x * cellSize, gridRows * cellSize, 0, origin);
      ctx.beginPath(); ctx.moveTo(start.x, start.y); ctx.lineTo(end.x, end.y); ctx.stroke();
    }

    for (let row = 0; row < gridRows; row++) {
      for (let col = gridCols - 1; col >= 0; col--) {
        const t = col / (gridCols - 1);
        const specIdx = Math.floor(Math.pow(t, 0.7) * (smoothSpectrum.length - 1));
        let specVal = 0, count = 0;
        for (let j = -2; j <= 2; j++) {
          specVal += smoothSpectrum[Math.max(0, Math.min(smoothSpectrum.length - 1, specIdx + j))];
          count++;
        }
        specVal = (specVal / count) * (0.6 + t * 0.6);
        const frozenVal = frozen[specIdx] || 0;
        let lfoInfluence = 0;
        if (lfoEnabled) {
          const dx = col / gridCols - (0.5 + lfoX * 0.5);
          const dy = row / gridRows - (0.5 + lfoY * 0.5);
          lfoInfluence = Math.max(0, 1 - Math.sqrt(dx * dx + dy * dy) * 2) * lfoDepth;
        }
        const height = 4 + row * 0.5 + specVal * maxHeight * (1 + freeze * 0.3) + frozenVal * freeze * 25 + lfoInfluence * 20;
        drawIsoColumn(col, row, height, cellSize, origin, specVal, frozenVal * freeze, lfoInfluence);
      }
    }

    const vignette = ctx.createRadialGradient(w/2, h/2, h * 0.2, w/2, h/2, h * 0.9);
    vignette.addColorStop(0, 'transparent');
    vignette.addColorStop(1, 'rgba(0, 2, 8, 0.8)');
    ctx.fillStyle = vignette;
    ctx.fillRect(0, 0, w, h);
  }

  onMount(() => {
    ctx = canvas.getContext('2d')!;
    resizeCanvas();
    render();
    window.addEventListener('resize', () => { ctx.resetTransform(); resizeCanvas(); });
    return () => cancelAnimationFrame(animationId);
  });

  function formatPercent(val: number) { return Math.round(val * 100) + '%'; }
  function formatBipolar(val: number) { const m = Math.round((val - 0.5) * 200); return m >= 0 ? `+${m}` : `${m}`; }
  function toggleLfoTarget(p: string) { const k = p as keyof typeof lfoTargets; lfoTargets[k].enabled = !lfoTargets[k].enabled; }
  function toggleLfoInvert(p: string) { const k = p as keyof typeof lfoTargets; lfoTargets[k].invert = !lfoTargets[k].invert; }
  function setLfoShape(id: string) { lfoShape = id as typeof lfoShape; }

  // Generate LFO path for preview - matches actual LFO movement
  function getLfoPath(shape: string, depth: number): string {
    const cx = 50, cy = 50, scale = 40;
    const points: string[] = [];
    const steps = 64;

    for (let i = 0; i <= steps; i++) {
      const t = (i / steps) * Math.PI * 2;
      let x = 0, y = 0;
      switch (shape) {
        case 'circle':
          x = Math.cos(t) * depth;
          y = Math.sin(t) * depth;
          break;
        case 'figure8':
          x = Math.sin(t) * depth;
          y = Math.sin(t * 2) * depth * 0.5;
          break;
        case 'wave':
          x = Math.sin(t) * depth;
          y = Math.sin(t * 1.5 + 1) * depth;
          break;
        case 'random':
          x = Math.sin(t * 0.7) * Math.cos(t * 1.3) * depth;
          y = Math.cos(t * 0.9) * Math.sin(t * 1.1) * depth;
          break;
      }
      const px = cx + x * scale;
      const py = cy + y * scale;
      points.push(i === 0 ? `M ${px} ${py}` : `L ${px} ${py}`);
    }
    return points.join(' ') + ' Z';
  }

  $: lfoPathD = getLfoPath(lfoShape, lfoDepth);
</script>

<canvas bind:this={canvas}></canvas>

<div class="header">
  <div class="logo">RIPPLE</div>
  <div class="subtitle">SPECTRAL TEXTURE</div>
</div>

<!-- Preset Selector -->
<div class="preset-container">
  <button class="preset-trigger" on:click={() => presetMenuOpen = !presetMenuOpen}>
    <span class="preset-label">PRESET</span>
    <span class="preset-value">{selectedPreset}</span>
    <svg class="chevron" class:open={presetMenuOpen} viewBox="0 0 12 12">
      <path d="M3 4.5l3 3 3-3" fill="none" stroke="currentColor" stroke-width="1.5"/>
    </svg>
  </button>

  {#if presetMenuOpen}
    <div class="preset-dropdown">
      {#each presets as preset}
        <button class="preset-option" class:selected={selectedPreset === preset.name} on:click={() => loadPreset(preset.name)}>
          {preset.name}
        </button>
      {/each}
    </div>
  {/if}
</div>

<!-- Controls Panel -->
<div class="controls-panel">
  {#each [
    { id: 'freeze', label: 'FREEZE', value: freeze, fmt: formatPercent },
    { id: 'sustain', label: 'SUSTAIN', value: sustain, fmt: formatPercent },
    { id: 'diffuse', label: 'DIFFUSE', value: diffuse, fmt: formatPercent },
    { id: 'shift', label: 'SHIFT', value: shift, fmt: formatBipolar },
    { id: 'tilt', label: 'TILT', value: tilt, fmt: formatBipolar },
    { id: 'feedback', label: 'FEEDBACK', value: feedback, fmt: formatPercent }
  ] as p}
    <div class="control">
      <div class="control-header">
        <span class="control-label">{p.label}</span>
        <span class="control-value">{p.fmt(p.value)}</span>
      </div>
      <div class="slider-container">
        <div class="slider-track">
          <div class="slider-fill" style="width: {p.value * 100}%"></div>
        </div>
        <input type="range" min="0" max="1" step="0.01"
          value={p.value}
          on:input={(e) => {
            const v = parseFloat(e.currentTarget.value);
            if (p.id === 'freeze') freeze = v;
            else if (p.id === 'sustain') sustain = v;
            else if (p.id === 'diffuse') diffuse = v;
            else if (p.id === 'shift') shift = v;
            else if (p.id === 'tilt') tilt = v;
            else if (p.id === 'feedback') feedback = v;
          }}
        />
      </div>
    </div>
  {/each}
</div>

<!-- LFO Button -->
<button class="lfo-btn" class:active={lfoPanelOpen || lfoEnabled} on:click={() => lfoPanelOpen = !lfoPanelOpen}>
  <svg viewBox="0 0 20 20"><path d="M2 10c1.5-3 3-3 4.5 0s3 3 4.5 0 3-3 4.5 0 3 3 4.5 0" fill="none" stroke="currentColor" stroke-width="1.5"/></svg>
  <span>RIPPLE</span>
</button>

<!-- LFO Panel -->
<div class="lfo-panel" class:open={lfoPanelOpen}>
  <div class="lfo-header">
    <span class="lfo-title">RIPPLE LFO</span>
    <button class="lfo-power" class:on={lfoEnabled} on:click={() => lfoEnabled = !lfoEnabled}>
      {lfoEnabled ? 'ON' : 'OFF'}
    </button>
  </div>

  <div class="lfo-shapes">
    {#each ['circle', 'figure8', 'wave', 'random'] as shape}
      <button class="shape-btn" class:selected={lfoShape === shape} on:click={() => setLfoShape(shape)}>
        {#if shape === 'circle'}<svg viewBox="0 0 20 20"><circle cx="10" cy="10" r="6" fill="none" stroke="currentColor" stroke-width="1.5"/></svg>
        {:else if shape === 'figure8'}<svg viewBox="0 0 20 20"><path d="M10 4c-2.5 0-4 1.5-4 3s1.5 3 4 3 4 1.5 4 3-1.5 3-4 3-4-1.5-4-3" fill="none" stroke="currentColor" stroke-width="1.5"/></svg>
        {:else if shape === 'wave'}<svg viewBox="0 0 20 20"><path d="M2 10c1.5-3 3-3 4.5 0s3 3 4.5 0 3-3 4.5 0" fill="none" stroke="currentColor" stroke-width="1.5"/></svg>
        {:else}<svg viewBox="0 0 20 20"><path d="M3 10h2l1.5-3 2.5 6 2-5 1.5 3h4" fill="none" stroke="currentColor" stroke-width="1.5"/></svg>
        {/if}
      </button>
    {/each}
  </div>

  <div class="lfo-sliders">
    <div class="lfo-slider">
      <span>RATE</span>
      <div class="slider-track"><div class="slider-fill" style="width: {(lfoRate - 0.05) / 0.95 * 100}%"></div></div>
      <input type="range" min="0.05" max="1" step="0.01" bind:value={lfoRate} />
    </div>
    <div class="lfo-slider">
      <span>DEPTH</span>
      <div class="slider-track"><div class="slider-fill" style="width: {lfoDepth * 100}%"></div></div>
      <input type="range" min="0" max="1" step="0.01" bind:value={lfoDepth} />
    </div>
  </div>

  <div class="lfo-targets">
    <span class="targets-label">TARGETS</span>
    <div class="target-grid">
      {#each Object.entries(lfoTargets) as [param, state]}
        <button class="target-btn" class:enabled={state.enabled} on:click={() => toggleLfoTarget(param)}>
          <span>{param.slice(0, 4).toUpperCase()}</span>
          {#if state.enabled}
            <button class="inv-btn" class:inv={state.invert} on:click|stopPropagation={() => toggleLfoInvert(param)}>±</button>
          {/if}
        </button>
      {/each}
    </div>
  </div>

  <div class="lfo-preview">
    <svg class="lfo-path" viewBox="0 0 100 100" preserveAspectRatio="xMidYMid meet">
      <line x1="50" y1="10" x2="50" y2="90" class="grid-line" />
      <line x1="10" y1="50" x2="90" y2="50" class="grid-line" />
      <path d={lfoPathD} />
    </svg>
    <div class="preview-dot" style="left: {50 + lfoX * 40}%; top: {50 + lfoY * 40}%"></div>
    <div class="preview-glow" style="left: {50 + lfoX * 40}%; top: {50 + lfoY * 40}%"></div>
  </div>

  <!-- Active targets indicator -->
  {#if lfoEnabled}
    <div class="active-targets">
      {#each Object.entries(lfoTargets).filter(([_, s]) => s.enabled) as [param, state]}
        <div class="target-pulse" class:inverted={state.invert}>
          <span>{param.slice(0, 4).toUpperCase()}</span>
          <div class="pulse-bar" style="transform: scaleX({0.5 + (state.invert ? -1 : 1) * (param === 'shift' || param === 'sustain' || param === 'feedback' ? lfoX : lfoY) * 0.5})"></div>
        </div>
      {/each}
    </div>
  {/if}
</div>

<style>
  :global(*) { box-sizing: border-box; }

  canvas { position: fixed; inset: 0; width: 100%; height: 100%; }

  .header {
    position: fixed; top: 28px; left: 0; right: 0;
    text-align: center; pointer-events: none; z-index: 10;
  }
  .logo {
    font: 100 52px system-ui, sans-serif;
    letter-spacing: 0.5em;
    background: linear-gradient(90deg,
      rgba(120, 200, 255, 0.95) 0%,
      rgba(100, 180, 255, 0.95) 30%,
      rgba(150, 220, 255, 0.95) 50%,
      rgba(100, 200, 255, 0.95) 70%,
      rgba(140, 180, 255, 0.9) 100%
    );
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
  }
  .subtitle {
    font: 500 11px system-ui, sans-serif;
    letter-spacing: 0.4em; color: rgba(140, 190, 230, 0.55); margin-top: 8px;
  }

  /* Glassmorphism base */
  .glass {
    background: rgba(8, 16, 28, 0.65);
    backdrop-filter: blur(20px);
    border: 1px solid rgba(100, 160, 220, 0.12);
  }

  /* Preset */
  .preset-container { position: fixed; top: 28px; left: 28px; z-index: 20; }

  .preset-trigger {
    display: flex; align-items: center; gap: 12px;
    padding: 10px 16px;
    background: rgba(8, 16, 28, 0.7); backdrop-filter: blur(20px);
    border: 1px solid rgba(100, 160, 220, 0.15);
    border-radius: 4px; cursor: pointer; transition: all 0.2s;
  }
  .preset-trigger:hover { border-color: rgba(100, 180, 255, 0.3); }
  .preset-label { font: 600 9px system-ui; letter-spacing: 0.15em; color: rgba(140, 180, 220, 0.5); }
  .preset-value { font: 500 12px system-ui; letter-spacing: 0.1em; color: rgba(180, 220, 255, 0.9); }
  .chevron { width: 12px; height: 12px; color: rgba(140, 190, 240, 0.5); transition: transform 0.2s; }
  .chevron.open { transform: rotate(180deg); }

  .preset-dropdown {
    position: absolute; top: 100%; left: 0; margin-top: 4px;
    min-width: 180px; padding: 6px;
    background: rgba(8, 16, 28, 0.95); backdrop-filter: blur(20px);
    border: 1px solid rgba(100, 160, 220, 0.15); border-radius: 4px;
    animation: slideDown 0.15s ease;
  }
  @keyframes slideDown { from { opacity: 0; transform: translateY(-8px); } }

  .preset-option {
    display: block; width: 100%; padding: 10px 14px;
    background: none; border: none; border-radius: 2px;
    font: 500 11px system-ui; letter-spacing: 0.08em;
    color: rgba(170, 210, 250, 0.8); text-align: left; cursor: pointer;
    transition: all 0.15s;
  }
  .preset-option:hover { background: rgba(60, 150, 130, 0.2); }
  .preset-option.selected { background: rgba(50, 160, 140, 0.3); color: rgba(180, 255, 230, 1); }

  /* Controls Panel */
  .controls-panel {
    position: fixed; bottom: 28px; left: 28px;
    display: flex; gap: 32px; padding: 22px 32px;
    background: rgba(8, 16, 28, 0.75); backdrop-filter: blur(20px);
    border: 1px solid rgba(100, 160, 220, 0.12); border-radius: 4px;
    z-index: 10;
  }

  .control { min-width: 100px; }
  .control-header { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 10px; }
  .control-label { font: 600 11px system-ui; letter-spacing: 0.1em; color: rgba(140, 190, 235, 0.8); }
  .control-value { font: 500 12px system-ui; color: rgba(120, 175, 230, 0.6); }

  .slider-container { position: relative; height: 5px; }
  .slider-track {
    position: absolute; inset: 0;
    background: rgba(40, 70, 110, 0.5); border-radius: 1px; overflow: hidden;
  }
  .slider-fill {
    height: 100%;
    background: linear-gradient(90deg,
      rgba(60, 180, 140, 0.85),
      rgba(80, 200, 180, 0.9),
      rgba(100, 200, 255, 0.95)
    );
  }
  .slider-container input {
    position: absolute; inset: -8px 0; width: 100%; height: 20px;
    opacity: 0; cursor: pointer;
  }

  /* LFO Button */
  .lfo-btn {
    position: fixed; bottom: 28px; right: 28px; z-index: 15;
    display: flex; align-items: center; gap: 10px; padding: 12px 18px;
    background: rgba(8, 16, 28, 0.7); backdrop-filter: blur(20px);
    border: 1px solid rgba(100, 160, 220, 0.12); border-radius: 4px;
    cursor: pointer; transition: all 0.2s;
  }
  .lfo-btn:hover { border-color: rgba(100, 180, 255, 0.25); }
  .lfo-btn.active { border-color: rgba(100, 180, 255, 0.35); }
  .lfo-btn svg { width: 20px; height: 20px; color: rgba(120, 200, 180, 0.6); }
  .lfo-btn.active svg { color: rgba(80, 220, 180, 0.95); }
  .lfo-btn span { font: 600 12px system-ui; letter-spacing: 0.12em; color: rgba(160, 210, 255, 0.8); }

  /* LFO Panel */
  .lfo-panel {
    position: fixed; bottom: 80px; right: 28px; z-index: 20;
    width: 260px; padding: 0;
    background: rgba(8, 16, 28, 0.85); backdrop-filter: blur(20px);
    border: 1px solid rgba(100, 160, 220, 0.12); border-radius: 4px;
    max-height: 0; overflow: hidden; opacity: 0;
    transform: translateY(16px);
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  }
  .lfo-panel.open { max-height: 550px; padding: 16px; opacity: 1; transform: translateY(0); }

  .lfo-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
  .lfo-title { font: 600 12px system-ui; letter-spacing: 0.1em; color: rgba(160, 200, 240, 0.85); }
  .lfo-power {
    padding: 6px 14px;
    background: rgba(20, 40, 65, 0.5); border: 1px solid rgba(80, 140, 200, 0.2);
    border-radius: 2px; font: 600 9px system-ui; letter-spacing: 0.1em;
    color: rgba(140, 190, 240, 0.5); cursor: pointer; transition: all 0.2s;
  }
  .lfo-power.on {
    background: rgba(50, 160, 130, 0.45); border-color: rgba(80, 220, 180, 0.5);
    color: rgba(180, 255, 220, 0.95); box-shadow: 0 0 15px rgba(80, 200, 160, 0.3);
  }

  .lfo-shapes { display: flex; gap: 6px; margin-bottom: 12px; }
  .shape-btn {
    flex: 1; padding: 6px;
    background: rgba(20, 40, 60, 0.4); border: 1px solid rgba(80, 140, 200, 0.12);
    border-radius: 3px; cursor: pointer; transition: all 0.15s;
  }
  .shape-btn svg { width: 20px; height: 20px; color: rgba(130, 180, 230, 0.4); }
  .shape-btn:hover { border-color: rgba(100, 170, 240, 0.25); }
  .shape-btn:hover svg { color: rgba(160, 210, 255, 0.6); }
  .shape-btn.selected { background: rgba(50, 150, 130, 0.4); border-color: rgba(80, 220, 180, 0.45); }
  .shape-btn.selected svg { color: rgba(150, 255, 220, 0.9); }

  .lfo-sliders { display: flex; flex-direction: column; gap: 10px; margin-bottom: 12px; }
  .lfo-slider { display: flex; align-items: center; gap: 14px; position: relative; }
  .lfo-slider span { font: 600 10px system-ui; letter-spacing: 0.1em; color: rgba(130, 180, 230, 0.6); width: 48px; }
  .lfo-slider .slider-track { flex: 1; height: 5px; position: relative; }
  .lfo-slider input { position: absolute; top: -8px; left: 48px; right: 0; height: 20px; opacity: 0; cursor: pointer; }

  .lfo-targets { margin-bottom: 10px; }
  .targets-label { display: block; font: 600 9px system-ui; letter-spacing: 0.1em; color: rgba(120, 170, 220, 0.5); margin-bottom: 8px; }
  .target-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 6px; }
  .target-btn {
    position: relative; padding: 8px 6px;
    background: rgba(25, 45, 70, 0.4); border: 1px solid rgba(80, 140, 200, 0.1);
    border-radius: 2px; cursor: pointer; transition: all 0.15s;
  }
  .target-btn span { font: 600 10px system-ui; letter-spacing: 0.05em; color: rgba(140, 190, 240, 0.5); }
  .target-btn:hover { border-color: rgba(100, 170, 240, 0.2); }
  .target-btn.enabled { background: rgba(50, 150, 130, 0.35); border-color: rgba(80, 220, 180, 0.4); }
  .target-btn.enabled span { color: rgba(150, 255, 220, 0.95); }
  .inv-btn {
    position: absolute; top: -3px; right: -3px; width: 14px; height: 14px;
    background: rgba(30, 50, 80, 0.9); border: 1px solid rgba(100, 160, 220, 0.25);
    border-radius: 2px; font: 700 8px system-ui; color: rgba(150, 200, 255, 0.6);
    cursor: pointer; transition: all 0.15s;
  }
  .inv-btn.inv { background: rgba(180, 100, 120, 0.5); border-color: rgba(255, 150, 170, 0.4); color: rgba(255, 200, 210, 0.9); }

  .lfo-preview {
    position: relative; width: 140px; height: 140px; margin: 0 auto 12px;
    background: rgba(8, 18, 32, 0.6); border: 1px solid rgba(70, 130, 190, 0.12);
    border-radius: 2px; overflow: hidden;
  }
  .lfo-path {
    position: absolute; inset: 0; width: 100%; height: 100%;
  }
  .lfo-path path {
    fill: none; stroke: rgba(100, 180, 255, 0.4); stroke-width: 1.5;
  }
  .lfo-path .grid-line {
    stroke: rgba(100, 160, 220, 0.1); stroke-width: 1;
  }
  .preview-dot {
    position: absolute; width: 10px; height: 10px;
    background: radial-gradient(circle, rgba(150, 255, 220, 1), rgba(80, 220, 180, 0.9));
    border-radius: 50%; transform: translate(-50%, -50%);
    box-shadow: 0 0 15px rgba(80, 220, 180, 0.8), 0 0 25px rgba(100, 200, 255, 0.4);
    z-index: 2;
  }
  .preview-glow {
    position: absolute; width: 35px; height: 35px;
    background: radial-gradient(circle, rgba(80, 220, 180, 0.4), rgba(100, 200, 255, 0.15) 50%, transparent 70%);
    border-radius: 50%; transform: translate(-50%, -50%);
    pointer-events: none;
  }
  .active-targets {
    display: flex; flex-direction: column; gap: 4px; margin-top: 10px;
  }
  .target-pulse {
    display: flex; align-items: center; gap: 8px;
    padding: 4px 8px; background: rgba(20, 40, 70, 0.4);
    border-radius: 2px; overflow: hidden;
  }
  .target-pulse span {
    font: 600 8px system-ui; letter-spacing: 0.05em;
    color: rgba(120, 220, 180, 0.8); width: 32px;
  }
  .target-pulse.inverted span { color: rgba(255, 160, 180, 0.7); }
  .pulse-bar {
    flex: 1; height: 3px;
    background: linear-gradient(90deg, rgba(60, 180, 140, 0.7), rgba(80, 220, 180, 0.95));
    transform-origin: left center;
  }
  .target-pulse.inverted .pulse-bar {
    background: linear-gradient(90deg, rgba(200, 100, 120, 0.6), rgba(255, 150, 180, 0.9));
  }

  /* Ice/Frost Texture Effects */
  @keyframes crystalGlow {
    0%, 100% {
      filter: drop-shadow(0 0 40px rgba(100, 180, 255, 0.5)) drop-shadow(0 0 80px rgba(120, 200, 255, 0.3));
    }
    50% {
      filter: drop-shadow(0 0 60px rgba(100, 180, 255, 0.7)) drop-shadow(0 0 100px rgba(140, 200, 255, 0.4));
    }
  }

  /* Frost texture - subtle ice crystal pattern inside panels */
  .controls-panel::before {
    content: ''; position: absolute; inset: 0;
    background:
      radial-gradient(ellipse 80px 40px at 15% 20%, rgba(140, 200, 255, 0.04) 0%, transparent 70%),
      radial-gradient(ellipse 60px 30px at 85% 70%, rgba(120, 180, 255, 0.03) 0%, transparent 70%),
      radial-gradient(ellipse 40px 60px at 50% 90%, rgba(100, 170, 255, 0.035) 0%, transparent 70%),
      linear-gradient(125deg, transparent 40%, rgba(150, 210, 255, 0.02) 50%, transparent 60%),
      linear-gradient(235deg, transparent 40%, rgba(130, 190, 255, 0.02) 50%, transparent 60%);
    pointer-events: none; border-radius: 4px;
  }
  .controls-panel::after {
    content: ''; position: absolute; inset: 0;
    background:
      repeating-linear-gradient(60deg, transparent, transparent 30px, rgba(150, 210, 255, 0.008) 30px, rgba(150, 210, 255, 0.008) 31px),
      repeating-linear-gradient(-60deg, transparent, transparent 25px, rgba(130, 200, 255, 0.006) 25px, rgba(130, 200, 255, 0.006) 26px);
    pointer-events: none; border-radius: 4px;
  }

  .lfo-panel::before {
    content: ''; position: absolute; inset: 0;
    background:
      radial-gradient(ellipse 50px 70px at 20% 30%, rgba(130, 190, 255, 0.05) 0%, transparent 70%),
      radial-gradient(ellipse 70px 40px at 80% 60%, rgba(140, 200, 255, 0.04) 0%, transparent 70%),
      linear-gradient(155deg, transparent 30%, rgba(150, 210, 255, 0.025) 50%, transparent 70%);
    pointer-events: none; border-radius: 4px;
  }
  .lfo-panel::after {
    content: ''; position: absolute; inset: 0;
    background:
      repeating-linear-gradient(70deg, transparent, transparent 20px, rgba(140, 200, 255, 0.01) 20px, rgba(140, 200, 255, 0.01) 21px),
      repeating-linear-gradient(-50deg, transparent, transparent 18px, rgba(120, 180, 255, 0.008) 18px, rgba(120, 180, 255, 0.008) 19px);
    pointer-events: none; border-radius: 4px;
  }

  .preset-dropdown::before {
    content: ''; position: absolute; inset: 0;
    background:
      radial-gradient(ellipse 60px 40px at 30% 25%, rgba(130, 190, 255, 0.04) 0%, transparent 70%),
      radial-gradient(ellipse 40px 50px at 70% 75%, rgba(140, 200, 255, 0.035) 0%, transparent 70%),
      repeating-linear-gradient(65deg, transparent, transparent 15px, rgba(150, 210, 255, 0.008) 15px, rgba(150, 210, 255, 0.008) 16px);
    pointer-events: none; border-radius: 4px;
  }
  .preset-dropdown { position: relative; }

  .logo { animation: crystalGlow 4s ease-in-out infinite; }

  .preset-trigger::after {
    content: ''; position: absolute; inset: 0;
    background:
      radial-gradient(ellipse 40px 20px at 20% 50%, rgba(140, 200, 255, 0.04) 0%, transparent 70%),
      radial-gradient(ellipse 30px 25px at 80% 50%, rgba(130, 190, 255, 0.03) 0%, transparent 70%);
    pointer-events: none;
  }
  .preset-trigger { position: relative; overflow: hidden; }
</style>

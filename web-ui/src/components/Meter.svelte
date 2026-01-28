<script lang="ts">
  export let label: string = '';
  export let value: number;

  $: dbValue = value > 0 ? 20 * Math.log10(value) : -60;
  $: fillHeight = Math.max(0, Math.min(100, (dbValue + 60) / 60 * 100));
  $: isClipping = dbValue > -0.1;
</script>

<div class="meter-container">
  {#if label}<div class="meter-label">{label}</div>{/if}
  <div class="meter">
    <div class="meter-scale">
      <span>0</span>
      <span>-6</span>
      <span>-12</span>
      <span>-24</span>
      <span>-48</span>
    </div>
    <div class="meter-track">
      <div
        class="meter-fill"
        class:clipping={isClipping}
        style="height: {fillHeight}%"
      ></div>
      <div class="meter-peak" style="bottom: {fillHeight}%"></div>
    </div>
  </div>
  <div class="meter-value">{dbValue.toFixed(1)} dB</div>
</div>

<style>
  .meter-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: var(--spacing-xs);
    height: 100%;
  }

  .meter-label {
    font-size: 9px;
    font-weight: 500;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--text-secondary);
  }

  .meter {
    display: flex;
    gap: 4px;
    height: 100%;
    min-height: 80px;
    flex: 1;
  }

  .meter-scale {
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    font-size: 8px;
    color: var(--text-muted);
  }

  .meter-track {
    position: relative;
    width: 12px;
    height: 100%;
    background: var(--bg-tertiary);
    border-radius: 2px;
    overflow: hidden;
  }

  .meter-fill {
    position: absolute;
    bottom: 0;
    left: 0;
    width: 100%;
    background: linear-gradient(
      to top,
      var(--accent-cyan-muted) 0%,
      var(--accent-cyan) 60%,
      var(--accent-magenta) 90%,
      #ff4444 100%
    );
    transition: height 50ms ease-out;
  }

  .meter-fill.clipping {
    background: linear-gradient(
      to top,
      var(--accent-magenta) 0%,
      #ff4444 100%
    );
  }

  .meter-peak {
    position: absolute;
    left: 0;
    width: 100%;
    height: 2px;
    background: var(--text-primary);
    transition: bottom 100ms ease-out;
  }

  .meter-value {
    font-family: var(--font-display);
    font-size: 9px;
    font-weight: 500;
    color: var(--text-muted);
  }
</style>

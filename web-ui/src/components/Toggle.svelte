<script lang="ts">
  import type { ToggleStore } from '../stores/params';

  export let store: ToggleStore;
  export let label: string = '';
  export let compact: boolean = false;

  $: isActive = $store;

  function handleClick() {
    store.toggle();
  }
</script>

<div class="toggle-container" class:compact>
  {#if label && !compact}
    <span class="toggle-label">{label}</span>
  {/if}

  <button
    class="toggle"
    class:active={isActive}
    on:click={handleClick}
    aria-pressed={isActive}
  >
    <span class="toggle-thumb"></span>
  </button>
</div>

<style>
  .toggle-container {
    display: flex;
    align-items: center;
    gap: var(--spacing-sm);
  }

  .toggle-container.compact {
    gap: var(--spacing-xs);
  }

  .toggle-label {
    font-size: 10px;
    font-weight: 500;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    color: var(--text-secondary);
  }

  .toggle {
    position: relative;
    width: 36px;
    height: 18px;
    padding: 0;
    border: 1px solid var(--border-subtle);
    border-radius: 9px;
    background: var(--bg-tertiary);
    cursor: pointer;
    transition: all var(--transition-fast);
  }

  .toggle-container.compact .toggle {
    width: 28px;
    height: 14px;
    border-radius: 7px;
  }

  .toggle:hover {
    border-color: var(--border-active);
  }

  .toggle.active {
    background: var(--accent-cyan-muted);
    border-color: var(--accent-cyan);
  }

  .toggle-thumb {
    position: absolute;
    top: 2px;
    left: 2px;
    width: 12px;
    height: 12px;
    border-radius: 50%;
    background: var(--text-muted);
    transition: all var(--transition-fast);
  }

  .toggle-container.compact .toggle-thumb {
    width: 8px;
    height: 8px;
  }

  .toggle.active .toggle-thumb {
    left: calc(100% - 14px);
    background: var(--text-primary);
    box-shadow: 0 0 8px var(--glow-cyan);
  }

  .toggle-container.compact .toggle.active .toggle-thumb {
    left: calc(100% - 10px);
  }
</style>

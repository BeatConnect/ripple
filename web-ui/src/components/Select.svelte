<script lang="ts">
  import type { ComboStore } from '../stores/params';

  export let label: string = '';
  export let store: ComboStore;
  export let options: string[] = [];

  // Subscribe to the nested stores properly
  $: indexValue = $store.index;
  $: choicesValue = $store.choices;
  $: displayChoices = options.length > 0 ? options : choicesValue;

  function handleChange(e: Event) {
    const target = e.target as HTMLSelectElement;
    store.setIndex(parseInt(target.value));
  }
</script>

<div class="select-container">
  {#if label}
    <span class="select-label">{label}</span>
  {/if}

  <div class="select-wrapper">
    <select
      value={indexValue}
      on:change={handleChange}
      class="select"
    >
      {#each displayChoices as choice, i}
        <option value={i}>{choice}</option>
      {/each}
    </select>
    <span class="select-arrow">
      <svg width="10" height="6" viewBox="0 0 10 6" fill="none">
        <path d="M1 1L5 5L9 1" stroke="currentColor" stroke-width="1.5" stroke-linecap="round"/>
      </svg>
    </span>
  </div>
</div>

<style>
  .select-container {
    display: flex;
    flex-direction: column;
    gap: var(--spacing-xs);
  }

  .select-label {
    font-size: 9px;
    font-weight: 500;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: var(--text-secondary);
  }

  .select-wrapper {
    position: relative;
  }

  .select {
    appearance: none;
    width: 100%;
    padding: var(--spacing-xs) var(--spacing-md);
    padding-right: 28px;
    font-family: var(--font-body);
    font-size: 11px;
    color: var(--text-primary);
    background: var(--bg-tertiary);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-sm);
    cursor: pointer;
    transition: all var(--transition-fast);
  }

  .select:hover {
    border-color: var(--border-active);
  }

  .select:focus {
    outline: none;
    border-color: var(--accent-cyan);
    box-shadow: 0 0 0 2px var(--glow-cyan);
  }

  .select option {
    background: var(--bg-secondary);
    color: var(--text-primary);
  }

  .select-arrow {
    position: absolute;
    right: 8px;
    top: 50%;
    transform: translateY(-50%);
    pointer-events: none;
    color: var(--text-muted);
  }
</style>

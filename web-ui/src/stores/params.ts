/**
 * Svelte Stores for JUCE Parameter Binding
 *
 * Reactive stores that sync with JUCE's native relay system.
 */

import { writable, derived, type Writable, type Readable } from 'svelte/store';
import {
  getSliderState,
  getToggleState,
  getComboBoxState,
  isInJuceWebView,
  type SliderState,
  type ToggleState,
  type ComboBoxState
} from '../lib/juce-bridge';

// =============================================================================
// Slider Parameter Store
// =============================================================================

export interface SliderStore extends Writable<number> {
  onDragStart: () => void;
  onDragEnd: () => void;
  isConnected: boolean;
}

export function createSliderStore(paramId: string, defaultValue = 0.5): SliderStore {
  const isConnected = isInJuceWebView();
  const state = getSliderState(paramId);

  const initialValue = isConnected ? state.getNormalisedValue() : defaultValue;
  const { subscribe, set, update } = writable(initialValue);

  // Listen for changes from JUCE
  if (isConnected) {
    state.valueChangedEvent.addListener(() => {
      set(state.getNormalisedValue());
    });
  }

  return {
    subscribe,
    set: (value: number) => {
      set(value);
      state.setNormalisedValue(value);
    },
    update,
    onDragStart: () => state.sliderDragStarted(),
    onDragEnd: () => state.sliderDragEnded(),
    isConnected
  };
}

// =============================================================================
// Toggle Parameter Store
// =============================================================================

export interface ToggleStore extends Writable<boolean> {
  toggle: () => void;
  isConnected: boolean;
}

export function createToggleStore(paramId: string, defaultValue = false): ToggleStore {
  const isConnected = isInJuceWebView();
  const state = getToggleState(paramId);

  const initialValue = isConnected ? state.getValue() : defaultValue;
  const { subscribe, set, update } = writable(initialValue);

  if (isConnected) {
    state.valueChangedEvent.addListener(() => {
      set(state.getValue());
    });
  }

  return {
    subscribe,
    set: (value: boolean) => {
      set(value);
      state.setValue(value);
    },
    update,
    toggle: () => {
      update((v) => {
        const newValue = !v;
        state.setValue(newValue);
        return newValue;
      });
    },
    isConnected
  };
}

// =============================================================================
// ComboBox Parameter Store
// =============================================================================

export interface ComboStore {
  index: Writable<number>;
  choices: Readable<string[]>;
  isConnected: boolean;
}

export function createComboStore(paramId: string, defaultIndex = 0): ComboStore {
  const isConnected = isInJuceWebView();
  const state = getComboBoxState(paramId);

  const initialIndex = isConnected ? state.getChoiceIndex() : defaultIndex;
  const initialChoices = isConnected ? state.getChoices() : [];

  const index = writable(initialIndex);
  const choices = writable(initialChoices);

  if (isConnected) {
    state.valueChangedEvent.addListener(() => {
      index.set(state.getChoiceIndex());
    });

    state.propertiesChangedEvent.addListener(() => {
      choices.set(state.getChoices());
    });
  }

  return {
    index: {
      subscribe: index.subscribe,
      set: (value: number) => {
        index.set(value);
        state.setChoiceIndex(value);
      },
      update: index.update
    },
    choices: { subscribe: choices.subscribe },
    isConnected
  };
}

// =============================================================================
// Visualizer Data Store
// =============================================================================

import { addCustomEventListener } from '../lib/juce-bridge';

export interface VisualizerData {
  inputLevel: number;
  outputLevel: number;
  rippleBands: number[];
  lfoValues: number[];
}

const defaultVisualizerData: VisualizerData = {
  inputLevel: 0,
  outputLevel: 0,
  rippleBands: new Array(16).fill(0.5),
  lfoValues: [0.5, 0.5, 0.5, 0.5]
};

export function createVisualizerStore() {
  const { subscribe, set } = writable<VisualizerData>(defaultVisualizerData);

  addCustomEventListener('visualizerData', (data) => {
    set(data as VisualizerData);
  });

  return { subscribe };
}

export const visualizerData = createVisualizerStore();

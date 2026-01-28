/**
 * Svelte Stores for JUCE Parameter Binding
 */

import { writable, type Writable } from 'svelte/store';
import {
  getSliderState,
  getToggleState,
  getComboBoxState,
  isInJuceWebView,
  addCustomEventListener
} from '../lib/juce-bridge';

// =============================================================================
// Slider Parameter Store
// =============================================================================

export interface SliderStore extends Writable<number> {
  onDragStart: () => void;
  onDragEnd: () => void;
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
    onDragEnd: () => state.sliderDragEnded()
  };
}

// =============================================================================
// Toggle Parameter Store
// =============================================================================

export interface ToggleStore extends Writable<boolean> {
  toggle: () => void;
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
    }
  };
}

// =============================================================================
// ComboBox Parameter Store - Simple version
// =============================================================================

export interface ComboState {
  index: number;
  choices: string[];
}

export interface ComboStore extends Writable<ComboState> {
  setIndex: (index: number) => void;
}

export function createComboStore(paramId: string, defaultIndex = 0): ComboStore {
  const isConnected = isInJuceWebView();
  const state = getComboBoxState(paramId);

  const initialState: ComboState = {
    index: isConnected ? state.getChoiceIndex() : defaultIndex,
    choices: isConnected ? state.getChoices() : []
  };

  const { subscribe, set, update } = writable(initialState);

  if (isConnected) {
    state.valueChangedEvent.addListener(() => {
      update(s => ({ ...s, index: state.getChoiceIndex() }));
    });

    state.propertiesChangedEvent.addListener(() => {
      update(s => ({ ...s, choices: state.getChoices() }));
    });
  }

  return {
    subscribe,
    set,
    update,
    setIndex: (index: number) => {
      update(s => ({ ...s, index }));
      state.setChoiceIndex(index);
    }
  };
}

// =============================================================================
// Visualizer Data Store
// =============================================================================

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

function createVisualizerStore() {
  const { subscribe, set } = writable<VisualizerData>(defaultVisualizerData);

  addCustomEventListener('visualizerData', (data) => {
    set(data as VisualizerData);
  });

  return { subscribe };
}

export const visualizerData = createVisualizerStore();

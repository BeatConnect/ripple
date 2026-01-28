<script lang="ts">
  import { onMount, createEventDispatcher } from 'svelte';
  import { addCustomEventListener, emitEvent } from '../lib/juce-bridge';

  const dispatch = createEventDispatcher();

  interface ActivationInfo {
    activationCode: string;
    machineId: string;
    activatedAt: string;
    currentActivations: number;
    maxActivations: number;
    isValid: boolean;
  }

  let screenState: 'checking' | 'input' | 'activating' | 'success' | 'error' = 'checking';
  let licenseKey = '';
  let errorMessage = '';
  let activationInfo: ActivationInfo | null = null;
  let showContent = false;

  onMount(() => {
    // Entrance animation
    setTimeout(() => showContent = true, 100);

    // Listen for activation state
    const unsubState = addCustomEventListener('activationState', (data: any) => {
      if (!data.isConfigured) {
        // Not configured - skip activation
        dispatch('activated');
        return;
      }

      if (data.isActivated && data.info?.isValid) {
        activationInfo = data.info;
        screenState = 'success';
        setTimeout(() => dispatch('activated'), 1500);
      } else {
        screenState = 'input';
      }
    });

    // Listen for activation result
    const unsubResult = addCustomEventListener('activationResult', (data: any) => {
      switch (data.status) {
        case 'valid':
        case 'already_active':
          activationInfo = data.info || null;
          screenState = 'success';
          setTimeout(() => dispatch('activated'), 1500);
          break;
        case 'invalid':
          errorMessage = 'Invalid license key. Please check and try again.';
          screenState = 'error';
          break;
        case 'revoked':
          errorMessage = 'This license has been revoked.';
          screenState = 'error';
          break;
        case 'max_reached':
          errorMessage = 'Maximum activations reached. Deactivate another device first.';
          screenState = 'error';
          break;
        case 'network_error':
          errorMessage = 'Network error. Please check your connection.';
          screenState = 'error';
          break;
        case 'server_error':
          errorMessage = 'Server error. Please try again later.';
          screenState = 'error';
          break;
        default:
          errorMessage = 'Activation failed. Please try again.';
          screenState = 'error';
      }
    });

    // Request current status
    emitEvent('getActivationStatus', {});

    return () => {
      unsubState();
      unsubResult();
    };
  });

  function handleActivate() {
    const code = licenseKey.trim();
    if (!code) return;

    screenState = 'activating';
    errorMessage = '';
    emitEvent('activateLicense', { code });
  }

  function handleKeyDown(e: KeyboardEvent) {
    if (e.key === 'Enter') {
      handleActivate();
    }
  }

  function handleRetry() {
    screenState = 'input';
    errorMessage = '';
  }
</script>

<div class="activation-screen" class:visible={showContent}>
  <!-- Animated background -->
  <div class="activation-bg">
    <div class="glow glow-1"></div>
    <div class="glow glow-2"></div>
    <div class="glow glow-3"></div>
  </div>

  <div class="activation-content">
    <!-- Logo -->
    <div class="activation-logo">
      <h1 class="activation-title">RIPPLE</h1>
      <div class="activation-subtitle">SPECTRAL TEXTURE ENGINE</div>
    </div>

    <!-- State area -->
    <div class="activation-state-area">
      {#if screenState === 'checking'}
        <div class="state-content">
          <div class="spinner"></div>
          <p>Checking license...</p>
        </div>
      {:else if screenState === 'input'}
        <div class="state-content">
          <p class="prompt">Enter your license key to activate</p>
          <input
            type="text"
            class="license-input"
            placeholder="XXXX-XXXX-XXXX-XXXX"
            bind:value={licenseKey}
            on:keydown={handleKeyDown}
            spellcheck="false"
          />
          <button
            class="activate-btn"
            on:click={handleActivate}
            disabled={!licenseKey.trim()}
          >
            Activate
          </button>
        </div>
      {:else if screenState === 'activating'}
        <div class="state-content">
          <div class="spinner"></div>
          <p>Activating...</p>
        </div>
      {:else if screenState === 'success'}
        <div class="state-content">
          <div class="checkmark">
            <svg viewBox="0 0 52 52">
              <circle class="checkmark-circle" cx="26" cy="26" r="25" fill="none"/>
              <path class="checkmark-check" fill="none" d="M14.1 27.2l7.1 7.2 16.7-16.8"/>
            </svg>
          </div>
          <p class="success-text">Activated!</p>
          {#if activationInfo}
            <p class="info-text">
              {activationInfo.currentActivations} of {activationInfo.maxActivations} activations used
            </p>
          {/if}
        </div>
      {:else if screenState === 'error'}
        <div class="state-content">
          <div class="error-icon">!</div>
          <p class="error-text">{errorMessage}</p>
          <button class="activate-btn secondary" on:click={handleRetry}>
            Try Again
          </button>
        </div>
      {/if}
    </div>
  </div>

  <div class="version">v1.0.0</div>
</div>

<style>
  .activation-screen {
    position: fixed;
    inset: 0;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    background: linear-gradient(135deg, #030508 0%, #0a1020 50%, #051015 100%);
    opacity: 0;
    transition: opacity 0.5s ease;
    z-index: 1000;
  }

  .activation-screen.visible {
    opacity: 1;
  }

  .activation-bg {
    position: absolute;
    inset: 0;
    overflow: hidden;
  }

  .glow {
    position: absolute;
    border-radius: 50%;
    filter: blur(80px);
    opacity: 0.4;
  }

  .glow-1 {
    width: 400px;
    height: 400px;
    background: radial-gradient(circle, rgba(60, 140, 200, 0.4) 0%, transparent 70%);
    top: -100px;
    left: -100px;
    animation: float1 8s ease-in-out infinite;
  }

  .glow-2 {
    width: 300px;
    height: 300px;
    background: radial-gradient(circle, rgba(100, 180, 255, 0.3) 0%, transparent 70%);
    bottom: -50px;
    right: -50px;
    animation: float2 10s ease-in-out infinite;
  }

  .glow-3 {
    width: 250px;
    height: 250px;
    background: radial-gradient(circle, rgba(80, 160, 220, 0.25) 0%, transparent 70%);
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    animation: float3 12s ease-in-out infinite;
  }

  @keyframes float1 {
    0%, 100% { transform: translate(0, 0); }
    50% { transform: translate(30px, 20px); }
  }

  @keyframes float2 {
    0%, 100% { transform: translate(0, 0); }
    50% { transform: translate(-20px, -30px); }
  }

  @keyframes float3 {
    0%, 100% { transform: translate(-50%, -50%) scale(1); }
    50% { transform: translate(-50%, -50%) scale(1.1); }
  }

  .activation-content {
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 48px;
  }

  .activation-logo {
    text-align: center;
  }

  .activation-title {
    font: 100 64px system-ui, sans-serif;
    letter-spacing: 0.4em;
    margin: 0;
    background: linear-gradient(90deg,
      rgba(120, 200, 255, 0.95) 0%,
      rgba(150, 220, 255, 0.95) 50%,
      rgba(100, 180, 255, 0.9) 100%
    );
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    filter: drop-shadow(0 0 40px rgba(100, 180, 255, 0.5));
  }

  .activation-subtitle {
    font: 500 12px system-ui, sans-serif;
    letter-spacing: 0.3em;
    color: rgba(140, 190, 230, 0.5);
    margin-top: 12px;
  }

  .activation-state-area {
    min-height: 200px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .state-content {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 20px;
    animation: fadeIn 0.3s ease;
  }

  @keyframes fadeIn {
    from { opacity: 0; transform: translateY(10px); }
    to { opacity: 1; transform: translateY(0); }
  }

  .state-content p {
    font: 400 14px system-ui, sans-serif;
    color: rgba(180, 210, 240, 0.7);
    margin: 0;
  }

  .prompt {
    font-size: 15px !important;
    color: rgba(160, 200, 240, 0.8) !important;
  }

  .spinner {
    width: 40px;
    height: 40px;
    border: 3px solid rgba(100, 180, 255, 0.2);
    border-top-color: rgba(100, 180, 255, 0.8);
    border-radius: 50%;
    animation: spin 1s linear infinite;
  }

  @keyframes spin {
    to { transform: rotate(360deg); }
  }

  .license-input {
    width: 320px;
    padding: 14px 20px;
    background: rgba(10, 20, 40, 0.8);
    border: 1px solid rgba(100, 160, 220, 0.2);
    border-radius: 4px;
    font: 500 16px monospace;
    letter-spacing: 0.1em;
    color: rgba(200, 230, 255, 0.95);
    text-align: center;
    outline: none;
    transition: all 0.2s;
  }

  .license-input::placeholder {
    color: rgba(100, 150, 200, 0.4);
  }

  .license-input:focus {
    border-color: rgba(100, 180, 255, 0.5);
    box-shadow: 0 0 20px rgba(100, 180, 255, 0.15);
  }

  .activate-btn {
    padding: 12px 40px;
    background: linear-gradient(135deg, rgba(60, 140, 200, 0.8) 0%, rgba(80, 160, 220, 0.8) 100%);
    border: 1px solid rgba(100, 180, 255, 0.3);
    border-radius: 4px;
    font: 600 13px system-ui, sans-serif;
    letter-spacing: 0.1em;
    color: rgba(220, 240, 255, 0.95);
    cursor: pointer;
    transition: all 0.2s;
  }

  .activate-btn:hover:not(:disabled) {
    background: linear-gradient(135deg, rgba(70, 150, 210, 0.9) 0%, rgba(90, 170, 230, 0.9) 100%);
    box-shadow: 0 0 25px rgba(100, 180, 255, 0.25);
  }

  .activate-btn:disabled {
    opacity: 0.4;
    cursor: not-allowed;
  }

  .activate-btn.secondary {
    background: rgba(30, 50, 80, 0.6);
  }

  .activate-btn.secondary:hover {
    background: rgba(40, 60, 90, 0.8);
  }

  .checkmark {
    width: 60px;
    height: 60px;
  }

  .checkmark svg {
    width: 100%;
    height: 100%;
  }

  .checkmark-circle {
    stroke: rgba(80, 200, 160, 0.8);
    stroke-width: 2;
    stroke-dasharray: 166;
    stroke-dashoffset: 166;
    animation: stroke 0.6s cubic-bezier(0.65, 0, 0.45, 1) forwards;
  }

  .checkmark-check {
    stroke: rgba(80, 200, 160, 0.95);
    stroke-width: 3;
    stroke-linecap: round;
    stroke-linejoin: round;
    stroke-dasharray: 48;
    stroke-dashoffset: 48;
    animation: stroke 0.3s cubic-bezier(0.65, 0, 0.45, 1) 0.4s forwards;
  }

  @keyframes stroke {
    100% { stroke-dashoffset: 0; }
  }

  .success-text {
    font-size: 18px !important;
    color: rgba(80, 200, 160, 0.95) !important;
    font-weight: 500;
  }

  .info-text {
    font-size: 12px !important;
    color: rgba(140, 180, 200, 0.6) !important;
  }

  .error-icon {
    width: 50px;
    height: 50px;
    border-radius: 50%;
    background: rgba(200, 80, 80, 0.2);
    border: 2px solid rgba(200, 80, 80, 0.6);
    display: flex;
    align-items: center;
    justify-content: center;
    font: 700 24px system-ui;
    color: rgba(255, 120, 120, 0.9);
  }

  .error-text {
    color: rgba(255, 140, 140, 0.9) !important;
    text-align: center;
    max-width: 300px;
  }

  .version {
    position: absolute;
    bottom: 20px;
    right: 24px;
    font: 400 11px system-ui;
    color: rgba(100, 140, 180, 0.3);
  }
</style>

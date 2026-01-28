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

  let screenState: 'checking' | 'input' | 'activating' | 'success' | 'transition' | 'error' = 'checking';
  let licenseKey = '';
  let errorMessage = '';
  let activationInfo: ActivationInfo | null = null;
  let showContent = false;
  let inputFocused = false;
  let transitionOut = false;

  onMount(() => {
    // Staggered entrance animation
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
        setTimeout(() => startTransitionOut(), 1500);
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
          setTimeout(() => startTransitionOut(), 1500);
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
    // DEV MODE: Skip check, go straight to input
    const DEV_MODE = true;
    if (DEV_MODE) {
      setTimeout(() => screenState = 'input', 500);
    } else {
      emitEvent('getActivationStatus', {});
    }

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

    // DEV MODE: Fake success for testing UI
    const DEV_MODE = true;
    if (DEV_MODE) {
      setTimeout(() => {
        activationInfo = {
          activationCode: code,
          machineId: 'TEST-MACHINE',
          activatedAt: new Date().toISOString(),
          currentActivations: 1,
          maxActivations: 3,
          isValid: true
        };
        screenState = 'success';
        setTimeout(() => startTransitionOut(), 1500);
      }, 1000);
      return;
    }

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

  function startTransitionOut() {
    transitionOut = true;
    // Let the transition animation play, then dispatch
    setTimeout(() => dispatch('activated'), 1200);
  }
</script>

<div class="activation-screen" class:visible={showContent} class:transition-out={transitionOut}>
  <!-- Animated particle field background -->
  <div class="particle-field">
    {#each Array(40) as _, i}
      <div
        class="particle"
        style="
          --delay: {i * 0.15}s;
          --x: {Math.random() * 100}%;
          --y: {Math.random() * 100}%;
          --size: {2 + Math.random() * 4}px;
          --duration: {8 + Math.random() * 12}s;
          --drift: {-30 + Math.random() * 60}px;
        "
      ></div>
    {/each}
  </div>

  <!-- Aurora waves -->
  <div class="aurora-container">
    <div class="aurora aurora-1"></div>
    <div class="aurora aurora-2"></div>
    <div class="aurora aurora-3"></div>
  </div>

  <!-- Radial glow -->
  <div class="center-glow"></div>

  <div class="activation-content">
    <!-- Logo with enhanced animation -->
    <div class="activation-logo" class:checking={screenState === 'checking'}>
      <div class="logo-glow"></div>
      <h1 class="activation-title">RIPPLE</h1>
      <div class="activation-subtitle">SPECTRAL TEXTURE ENGINE</div>
      <div class="logo-line"></div>
    </div>

    <!-- State area with transitions -->
    <div class="activation-state-area">
      {#if screenState === 'checking'}
        <div class="state-content state-checking">
          <div class="pulse-ring">
            <div class="pulse-core"></div>
          </div>
          <p class="state-text">Verifying license...</p>
        </div>
      {:else if screenState === 'input'}
        <div class="state-content state-input">
          <p class="prompt">Enter your license key to unlock</p>
          <div class="input-wrapper" class:focused={inputFocused}>
            <div class="input-glow"></div>
            <input
              type="text"
              class="license-input"
              placeholder="XXXX-XXXX-XXXX-XXXX"
              bind:value={licenseKey}
              on:keydown={handleKeyDown}
              on:focus={() => inputFocused = true}
              on:blur={() => inputFocused = false}
              spellcheck="false"
              autocomplete="off"
            />
          </div>
          <button
            class="activate-btn"
            on:click={handleActivate}
            disabled={!licenseKey.trim()}
          >
            <span class="btn-text">Activate</span>
            <div class="btn-shine"></div>
          </button>
        </div>
      {:else if screenState === 'activating'}
        <div class="state-content state-activating">
          <div class="loading-orb">
            <div class="orb-ring orb-ring-1"></div>
            <div class="orb-ring orb-ring-2"></div>
            <div class="orb-ring orb-ring-3"></div>
            <div class="orb-core"></div>
          </div>
          <p class="state-text">Activating your license...</p>
        </div>
      {:else if screenState === 'success'}
        <div class="state-content state-success">
          <div class="success-burst">
            {#each Array(12) as _, i}
              <div class="burst-ray" style="--angle: {i * 30}deg"></div>
            {/each}
          </div>
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
          <p class="loading-text">Loading Ripple...</p>
        </div>
      {:else if screenState === 'error'}
        <div class="state-content state-error">
          <div class="error-icon">
            <svg viewBox="0 0 24 24">
              <circle cx="12" cy="12" r="11" fill="none" stroke="currentColor" stroke-width="1.5"/>
              <line x1="12" y1="7" x2="12" y2="13" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
              <circle cx="12" cy="17" r="1.5" fill="currentColor"/>
            </svg>
          </div>
          <p class="error-text">{errorMessage}</p>
          <button class="activate-btn secondary" on:click={handleRetry}>
            <span class="btn-text">Try Again</span>
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
    background: linear-gradient(135deg, #010306 0%, #040812 40%, #061020 70%, #030810 100%);
    opacity: 0;
    transition: opacity 0.8s cubic-bezier(0.4, 0, 0.2, 1);
    z-index: 1000;
    overflow: hidden;
  }

  .activation-screen.visible {
    opacity: 1;
  }

  /* Particle Field */
  .particle-field {
    position: absolute;
    inset: 0;
    overflow: hidden;
  }

  .particle {
    position: absolute;
    left: var(--x);
    top: var(--y);
    width: var(--size);
    height: var(--size);
    background: radial-gradient(circle, rgba(100, 180, 255, 0.8) 0%, rgba(120, 200, 255, 0.4) 50%, transparent 70%);
    border-radius: 50%;
    animation: particleFloat var(--duration) ease-in-out infinite;
    animation-delay: var(--delay);
    opacity: 0;
  }

  @keyframes particleFloat {
    0%, 100% {
      transform: translateY(0) translateX(0);
      opacity: 0;
    }
    10% {
      opacity: 0.6;
    }
    50% {
      transform: translateY(-100px) translateX(var(--drift));
      opacity: 0.8;
    }
    90% {
      opacity: 0.4;
    }
  }

  /* Aurora Waves */
  .aurora-container {
    position: absolute;
    inset: 0;
    overflow: hidden;
  }

  .aurora {
    position: absolute;
    width: 200%;
    height: 300px;
    left: -50%;
    filter: blur(60px);
    opacity: 0.35;
  }

  .aurora-1 {
    top: -100px;
    background: linear-gradient(90deg,
      transparent 0%,
      rgba(60, 140, 200, 0.4) 20%,
      rgba(100, 180, 255, 0.5) 40%,
      rgba(80, 160, 220, 0.4) 60%,
      transparent 80%
    );
    animation: auroraWave1 15s ease-in-out infinite;
  }

  .aurora-2 {
    top: 50px;
    background: linear-gradient(90deg,
      transparent 10%,
      rgba(80, 160, 220, 0.3) 30%,
      rgba(120, 200, 255, 0.4) 50%,
      rgba(90, 170, 230, 0.3) 70%,
      transparent 90%
    );
    animation: auroraWave2 18s ease-in-out infinite;
    animation-delay: -5s;
  }

  .aurora-3 {
    bottom: -50px;
    background: linear-gradient(90deg,
      transparent 5%,
      rgba(70, 150, 210, 0.25) 25%,
      rgba(110, 190, 250, 0.35) 45%,
      rgba(85, 165, 225, 0.25) 65%,
      transparent 85%
    );
    animation: auroraWave3 20s ease-in-out infinite;
    animation-delay: -10s;
  }

  @keyframes auroraWave1 {
    0%, 100% { transform: translateX(-10%) skewX(-5deg); }
    50% { transform: translateX(10%) skewX(5deg); }
  }

  @keyframes auroraWave2 {
    0%, 100% { transform: translateX(15%) skewX(3deg); }
    50% { transform: translateX(-15%) skewX(-3deg); }
  }

  @keyframes auroraWave3 {
    0%, 100% { transform: translateX(-5%) skewX(-2deg); }
    50% { transform: translateX(5%) skewX(2deg); }
  }

  /* Center Glow */
  .center-glow {
    position: absolute;
    width: 600px;
    height: 600px;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    background: radial-gradient(circle,
      rgba(80, 160, 220, 0.15) 0%,
      rgba(100, 180, 255, 0.08) 30%,
      transparent 70%
    );
    animation: centerPulse 4s ease-in-out infinite;
  }

  @keyframes centerPulse {
    0%, 100% { transform: translate(-50%, -50%) scale(1); opacity: 0.8; }
    50% { transform: translate(-50%, -50%) scale(1.1); opacity: 1; }
  }

  .activation-content {
    position: relative;
    z-index: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 60px;
  }

  /* Logo */
  .activation-logo {
    text-align: center;
    position: relative;
    animation: logoEnter 1s cubic-bezier(0.4, 0, 0.2, 1) forwards;
    opacity: 0;
    transform: translateY(-20px);
  }

  @keyframes logoEnter {
    to { opacity: 1; transform: translateY(0); }
  }

  .activation-logo.checking {
    animation: logoEnter 1s cubic-bezier(0.4, 0, 0.2, 1) forwards, logoPulse 2s ease-in-out infinite 1s;
  }

  @keyframes logoPulse {
    0%, 100% { filter: drop-shadow(0 0 40px rgba(100, 180, 255, 0.5)); }
    50% { filter: drop-shadow(0 0 60px rgba(100, 180, 255, 0.8)); }
  }

  .logo-glow {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%);
    width: 400px;
    height: 150px;
    background: radial-gradient(ellipse, rgba(100, 180, 255, 0.2) 0%, transparent 70%);
    filter: blur(30px);
    pointer-events: none;
  }

  .activation-title {
    font: 100 72px system-ui, -apple-system, sans-serif;
    letter-spacing: 0.45em;
    margin: 0;
    background: linear-gradient(90deg,
      rgba(100, 180, 255, 0.9) 0%,
      rgba(140, 210, 255, 1) 25%,
      rgba(180, 230, 255, 1) 50%,
      rgba(140, 210, 255, 1) 75%,
      rgba(100, 180, 255, 0.9) 100%
    );
    background-size: 200% 100%;
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    background-clip: text;
    animation: titleShimmer 8s linear infinite;
    filter: drop-shadow(0 0 30px rgba(100, 180, 255, 0.4));
  }

  @keyframes titleShimmer {
    0% { background-position: 100% 0; }
    100% { background-position: -100% 0; }
  }

  .activation-subtitle {
    font: 500 12px system-ui, sans-serif;
    letter-spacing: 0.35em;
    color: rgba(130, 190, 235, 0.6);
    margin-top: 16px;
    animation: subtitleEnter 0.8s cubic-bezier(0.4, 0, 0.2, 1) 0.3s forwards;
    opacity: 0;
  }

  @keyframes subtitleEnter {
    to { opacity: 1; }
  }

  .logo-line {
    width: 120px;
    height: 1px;
    margin: 20px auto 0;
    background: linear-gradient(90deg, transparent, rgba(100, 180, 255, 0.5), transparent);
    animation: lineGrow 0.8s cubic-bezier(0.4, 0, 0.2, 1) 0.5s forwards;
    transform: scaleX(0);
  }

  @keyframes lineGrow {
    to { transform: scaleX(1); }
  }

  /* State Area */
  .activation-state-area {
    min-height: 220px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .state-content {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 24px;
    animation: stateEnter 0.5s cubic-bezier(0.4, 0, 0.2, 1);
  }

  @keyframes stateEnter {
    from { opacity: 0; transform: translateY(15px) scale(0.95); }
    to { opacity: 1; transform: translateY(0) scale(1); }
  }

  .state-text {
    font: 400 15px system-ui, sans-serif;
    color: rgba(160, 200, 240, 0.8);
    letter-spacing: 0.05em;
    margin: 0;
  }

  /* Checking State - Pulse Ring */
  .pulse-ring {
    position: relative;
    width: 60px;
    height: 60px;
  }

  .pulse-ring::before,
  .pulse-ring::after {
    content: '';
    position: absolute;
    inset: 0;
    border: 2px solid rgba(100, 180, 255, 0.4);
    border-radius: 50%;
    animation: pulseExpand 2s ease-out infinite;
  }

  .pulse-ring::after {
    animation-delay: 1s;
  }

  @keyframes pulseExpand {
    0% { transform: scale(0.5); opacity: 1; }
    100% { transform: scale(2); opacity: 0; }
  }

  .pulse-core {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 16px;
    height: 16px;
    transform: translate(-50%, -50%);
    background: radial-gradient(circle, rgba(120, 200, 255, 1) 0%, rgba(100, 180, 255, 0.6) 100%);
    border-radius: 50%;
    box-shadow: 0 0 20px rgba(100, 180, 255, 0.6);
    animation: corePulse 1s ease-in-out infinite;
  }

  @keyframes corePulse {
    0%, 100% { transform: translate(-50%, -50%) scale(1); }
    50% { transform: translate(-50%, -50%) scale(1.2); }
  }

  /* Input State */
  .prompt {
    font: 400 16px system-ui, sans-serif;
    color: rgba(150, 200, 245, 0.85);
    letter-spacing: 0.03em;
    margin: 0;
  }

  .input-wrapper {
    position: relative;
    width: 360px;
  }

  .input-glow {
    position: absolute;
    inset: -2px;
    background: linear-gradient(135deg, rgba(80, 160, 220, 0.3), rgba(120, 200, 255, 0.2));
    border-radius: 6px;
    filter: blur(8px);
    opacity: 0;
    transition: opacity 0.3s;
  }

  .input-wrapper.focused .input-glow {
    opacity: 1;
  }

  .license-input {
    width: 100%;
    padding: 16px 24px;
    background: rgba(8, 18, 35, 0.9);
    border: 1px solid rgba(100, 160, 220, 0.25);
    border-radius: 6px;
    font: 500 18px 'SF Mono', 'Fira Code', Consolas, monospace;
    letter-spacing: 0.15em;
    color: rgba(200, 230, 255, 0.95);
    text-align: center;
    outline: none;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    position: relative;
    z-index: 1;
  }

  .license-input::placeholder {
    color: rgba(100, 150, 200, 0.4);
    letter-spacing: 0.12em;
  }

  .license-input:focus {
    border-color: rgba(100, 180, 255, 0.6);
    box-shadow: 0 0 30px rgba(100, 180, 255, 0.15), inset 0 0 20px rgba(100, 180, 255, 0.05);
  }

  .activate-btn {
    position: relative;
    padding: 14px 48px;
    background: linear-gradient(135deg, rgba(60, 140, 200, 0.85) 0%, rgba(80, 170, 240, 0.85) 100%);
    border: 1px solid rgba(100, 180, 255, 0.4);
    border-radius: 6px;
    cursor: pointer;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    overflow: hidden;
  }

  .btn-text {
    position: relative;
    z-index: 1;
    font: 600 14px system-ui, sans-serif;
    letter-spacing: 0.12em;
    color: rgba(220, 245, 255, 0.98);
  }

  .btn-shine {
    position: absolute;
    top: 0;
    left: -100%;
    width: 100%;
    height: 100%;
    background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  }

  .activate-btn:hover:not(:disabled) {
    transform: translateY(-2px);
    box-shadow: 0 8px 30px rgba(80, 160, 220, 0.4);
    background: linear-gradient(135deg, rgba(70, 150, 210, 0.95) 0%, rgba(90, 180, 250, 0.95) 100%);
  }

  .activate-btn:hover:not(:disabled) .btn-shine {
    animation: buttonShine 0.6s ease;
  }

  @keyframes buttonShine {
    to { left: 100%; }
  }

  .activate-btn:disabled {
    opacity: 0.4;
    cursor: not-allowed;
    transform: none;
  }

  .activate-btn.secondary {
    background: rgba(30, 50, 80, 0.7);
    border-color: rgba(100, 160, 220, 0.25);
  }

  .activate-btn.secondary:hover {
    background: rgba(40, 65, 100, 0.85);
  }

  /* Activating State - Loading Orb */
  .loading-orb {
    position: relative;
    width: 80px;
    height: 80px;
  }

  .orb-ring {
    position: absolute;
    inset: 0;
    border: 2px solid transparent;
    border-radius: 50%;
  }

  .orb-ring-1 {
    border-top-color: rgba(100, 180, 255, 0.8);
    animation: orbSpin 1.2s linear infinite;
  }

  .orb-ring-2 {
    inset: 8px;
    border-right-color: rgba(120, 200, 255, 0.6);
    animation: orbSpin 1.5s linear infinite reverse;
  }

  .orb-ring-3 {
    inset: 16px;
    border-bottom-color: rgba(140, 220, 255, 0.4);
    animation: orbSpin 2s linear infinite;
  }

  @keyframes orbSpin {
    to { transform: rotate(360deg); }
  }

  .orb-core {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 20px;
    height: 20px;
    transform: translate(-50%, -50%);
    background: radial-gradient(circle, rgba(150, 220, 255, 1) 0%, rgba(100, 180, 255, 0.6) 100%);
    border-radius: 50%;
    box-shadow: 0 0 25px rgba(100, 180, 255, 0.7);
    animation: orbCorePulse 0.8s ease-in-out infinite;
  }

  @keyframes orbCorePulse {
    0%, 100% { transform: translate(-50%, -50%) scale(1); opacity: 0.8; }
    50% { transform: translate(-50%, -50%) scale(1.15); opacity: 1; }
  }

  /* Success State */
  .state-success {
    position: relative;
  }

  .success-burst {
    position: absolute;
    top: 0;
    left: 50%;
    transform: translateX(-50%);
    width: 120px;
    height: 120px;
  }

  .burst-ray {
    position: absolute;
    top: 50%;
    left: 50%;
    width: 60px;
    height: 2px;
    background: linear-gradient(90deg, rgba(80, 200, 160, 0.8), transparent);
    transform-origin: left center;
    transform: rotate(var(--angle));
    animation: burstExpand 0.6s ease-out forwards;
    opacity: 0;
  }

  @keyframes burstExpand {
    0% { width: 0; opacity: 0; }
    50% { opacity: 1; }
    100% { width: 60px; opacity: 0; }
  }

  .checkmark {
    width: 70px;
    height: 70px;
    position: relative;
    z-index: 1;
  }

  .checkmark svg {
    width: 100%;
    height: 100%;
    filter: drop-shadow(0 0 20px rgba(80, 200, 160, 0.5));
  }

  .checkmark-circle {
    stroke: rgba(80, 200, 160, 0.8);
    stroke-width: 2;
    stroke-dasharray: 166;
    stroke-dashoffset: 166;
    animation: circleStroke 0.6s cubic-bezier(0.65, 0, 0.45, 1) forwards;
  }

  .checkmark-check {
    stroke: rgba(100, 220, 180, 0.95);
    stroke-width: 3;
    stroke-linecap: round;
    stroke-linejoin: round;
    stroke-dasharray: 48;
    stroke-dashoffset: 48;
    animation: checkStroke 0.4s cubic-bezier(0.65, 0, 0.45, 1) 0.4s forwards;
  }

  @keyframes circleStroke {
    100% { stroke-dashoffset: 0; }
  }

  @keyframes checkStroke {
    100% { stroke-dashoffset: 0; }
  }

  .success-text {
    font: 500 20px system-ui, sans-serif !important;
    color: rgba(100, 220, 180, 0.95) !important;
    letter-spacing: 0.08em;
    animation: successTextEnter 0.5s ease 0.5s forwards;
    opacity: 0;
  }

  @keyframes successTextEnter {
    to { opacity: 1; }
  }

  .info-text {
    font: 400 12px system-ui, sans-serif !important;
    color: rgba(130, 180, 210, 0.6) !important;
    animation: infoEnter 0.5s ease 0.7s forwards;
    opacity: 0;
  }

  @keyframes infoEnter {
    to { opacity: 1; }
  }

  .loading-text {
    font: 400 13px system-ui, sans-serif !important;
    color: rgba(140, 190, 240, 0.5) !important;
    animation: loadingTextPulse 1.5s ease-in-out infinite 0.8s;
    opacity: 0;
  }

  @keyframes loadingTextPulse {
    0%, 100% { opacity: 0.3; }
    50% { opacity: 0.7; }
  }

  /* Error State */
  .error-icon {
    width: 60px;
    height: 60px;
    color: rgba(255, 120, 130, 0.9);
    animation: errorShake 0.5s ease;
  }

  .error-icon svg {
    width: 100%;
    height: 100%;
    filter: drop-shadow(0 0 15px rgba(255, 100, 120, 0.4));
  }

  @keyframes errorShake {
    0%, 100% { transform: translateX(0); }
    20% { transform: translateX(-8px); }
    40% { transform: translateX(8px); }
    60% { transform: translateX(-6px); }
    80% { transform: translateX(6px); }
  }

  .error-text {
    font: 400 14px system-ui, sans-serif !important;
    color: rgba(255, 150, 160, 0.9) !important;
    text-align: center;
    max-width: 320px;
    line-height: 1.5;
  }

  /* Version */
  .version {
    position: absolute;
    bottom: 24px;
    right: 28px;
    font: 400 11px system-ui, sans-serif;
    color: rgba(100, 140, 180, 0.25);
    letter-spacing: 0.05em;
  }

  /* Transition Out Effect - Simple Fade */
  .activation-screen.transition-out {
    animation: fadeOut 1s ease-out forwards;
    pointer-events: none;
  }

  @keyframes fadeOut {
    0% { opacity: 1; }
    100% { opacity: 0; }
  }
</style>

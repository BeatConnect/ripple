<script lang="ts">
  import { onMount } from 'svelte';
  import * as THREE from 'three';
  import { visualizerData } from '../stores/params';

  export let xyX: number = 0.5;
  export let xyY: number = 0.5;
  export let zoneName: string = 'DRIFT';

  let container: HTMLDivElement;

  // Zone colors - muted, sophisticated
  const zoneColors: Record<string, [number, number, number]> = {
    'DRIFT': [0.25, 0.55, 0.58],
    'SHIMMER': [0.5, 0.35, 0.6],
    'PULSE': [0.6, 0.28, 0.45],
    'CHAOS': [0.65, 0.4, 0.3]
  };

  onMount(() => {
    if (!container) return;

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 0.1, 1000);
    camera.position.set(0, 15, 35);
    camera.lookAt(0, 0, 0);

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: false });
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.setClearColor(0x08080c, 1);
    container.appendChild(renderer.domElement);

    // Particle system
    const particleCount = 3000;
    const positions = new Float32Array(particleCount * 3);
    const randoms = new Float32Array(particleCount);

    for (let i = 0; i < particleCount; i++) {
      positions[i * 3] = (Math.random() - 0.5) * 100;
      positions[i * 3 + 1] = (Math.random() - 0.5) * 50;
      positions[i * 3 + 2] = (Math.random() - 0.5) * 100;
      randoms[i] = Math.random();
    }

    const particleGeometry = new THREE.BufferGeometry();
    particleGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    particleGeometry.setAttribute('aRandom', new THREE.BufferAttribute(randoms, 1));

    const particleMaterial = new THREE.ShaderMaterial({
      uniforms: {
        uTime: { value: 0 },
        uAudioLevel: { value: 0 },
        uXY: { value: new THREE.Vector2(0.5, 0.5) },
        uColor: { value: new THREE.Vector3(0.25, 0.55, 0.58) }
      },
      vertexShader: `
        attribute float aRandom;
        varying float vAlpha;
        varying float vRandom;
        uniform float uTime;
        uniform float uAudioLevel;
        uniform vec2 uXY;

        void main() {
          vRandom = aRandom;
          vec3 pos = position;

          // Speed and intensity from XY
          float speed = 0.2 + uXY.x * 1.5;
          float intensity = 0.5 + uXY.y * 4.0;

          // Ripple waves
          float dist = length(pos.xz);
          float wave = sin(dist * 0.08 - uTime * speed * 2.0) * intensity;
          wave += sin(pos.x * 0.05 + uTime * speed) * cos(pos.z * 0.05 + uTime * speed * 0.7) * intensity * 0.5;

          pos.y += wave * (1.0 + uAudioLevel * 3.0);

          // Audio reactive displacement
          pos.y += sin(aRandom * 6.28 + uTime * 2.0) * uAudioLevel * 5.0;

          vec4 mvPosition = modelViewMatrix * vec4(pos, 1.0);

          float size = 2.0 + aRandom * 2.0;
          size *= 1.0 + uAudioLevel * 2.0;
          size *= 1.0 + intensity * 0.3;

          gl_PointSize = size * (25.0 / -mvPosition.z);
          gl_Position = projectionMatrix * mvPosition;

          // Fade based on distance
          vAlpha = smoothstep(100.0, 20.0, -mvPosition.z) * (0.4 + uAudioLevel * 0.4);
        }
      `,
      fragmentShader: `
        varying float vAlpha;
        varying float vRandom;
        uniform vec3 uColor;
        uniform float uAudioLevel;

        void main() {
          float dist = length(gl_PointCoord - vec2(0.5));
          if (dist > 0.5) discard;

          float alpha = (1.0 - dist * 2.0) * vAlpha;

          // Color variation
          vec3 color = uColor;
          color += vRandom * 0.15;
          color += uAudioLevel * 0.2;

          // Glow
          float glow = exp(-dist * 3.0) * 0.5;
          color += glow * uColor;

          gl_FragColor = vec4(color, alpha);
        }
      `,
      transparent: true,
      blending: THREE.AdditiveBlending,
      depthWrite: false
    });

    const particles = new THREE.Points(particleGeometry, particleMaterial);
    scene.add(particles);

    // Grid floor
    const gridGeometry = new THREE.PlaneGeometry(120, 120, 60, 60);

    const gridMaterial = new THREE.ShaderMaterial({
      uniforms: {
        uTime: { value: 0 },
        uXY: { value: new THREE.Vector2(0.5, 0.5) },
        uColor: { value: new THREE.Vector3(0.25, 0.55, 0.58) },
        uAudioLevel: { value: 0 }
      },
      vertexShader: `
        varying vec2 vUv;
        varying float vElevation;
        uniform float uTime;
        uniform vec2 uXY;
        uniform float uAudioLevel;

        void main() {
          vUv = uv;
          vec3 pos = position;

          float speed = 0.3 + uXY.x * 1.2;
          float intensity = uXY.y * 3.0;

          float wave = sin(pos.x * 0.1 + uTime * speed) * sin(pos.y * 0.1 + uTime * speed * 0.8);
          float ripple = sin(length(pos.xy) * 0.15 - uTime * speed * 1.5);

          float elevation = (wave + ripple * 0.5) * intensity * (1.0 + uAudioLevel * 2.0);
          pos.z += elevation;
          vElevation = elevation;

          gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
        }
      `,
      fragmentShader: `
        varying vec2 vUv;
        varying float vElevation;
        uniform vec3 uColor;
        uniform float uAudioLevel;

        void main() {
          // Grid lines
          vec2 grid = abs(fract(vUv * 60.0 - 0.5) - 0.5);
          float line = min(grid.x, grid.y);
          float lineIntensity = 1.0 - smoothstep(0.0, 0.02, line);

          // Fade edges
          float fade = smoothstep(0.0, 0.15, vUv.x) * smoothstep(1.0, 0.85, vUv.x);
          fade *= smoothstep(0.0, 0.15, vUv.y) * smoothstep(1.0, 0.85, vUv.y);

          // Color based on elevation
          vec3 color = uColor * (0.3 + lineIntensity * 0.7);
          color += abs(vElevation) * 0.1 * uColor;
          color += uAudioLevel * 0.15;

          float alpha = fade * (0.2 + lineIntensity * 0.5 + uAudioLevel * 0.2);

          gl_FragColor = vec4(color, alpha);
        }
      `,
      transparent: true,
      side: THREE.DoubleSide,
      blending: THREE.AdditiveBlending,
      depthWrite: false
    });

    const grid = new THREE.Mesh(gridGeometry, gridMaterial);
    grid.rotation.x = -Math.PI * 0.5;
    grid.position.y = -12;
    scene.add(grid);

    let animationId: number;
    let currentAudioLevel = 0;
    let targetColor = new THREE.Vector3(0.25, 0.55, 0.58);
    let currentColor = new THREE.Vector3(0.25, 0.55, 0.58);

    function animate() {
      animationId = requestAnimationFrame(animate);

      const time = performance.now() * 0.001;

      // Smooth audio level
      let audioLevel = 0;
      visualizerData.subscribe(data => {
        audioLevel = Math.max(data.inputLevel, data.outputLevel);
      })();
      currentAudioLevel += (audioLevel - currentAudioLevel) * 0.15;

      // Smooth color transition
      const zc = zoneColors[zoneName] || zoneColors['DRIFT'];
      targetColor.set(zc[0], zc[1], zc[2]);
      currentColor.lerp(targetColor, 0.05);

      // Update uniforms
      particleMaterial.uniforms.uTime.value = time;
      particleMaterial.uniforms.uAudioLevel.value = currentAudioLevel;
      particleMaterial.uniforms.uXY.value.set(xyX, xyY);
      particleMaterial.uniforms.uColor.value.copy(currentColor);

      gridMaterial.uniforms.uTime.value = time;
      gridMaterial.uniforms.uXY.value.set(xyX, xyY);
      gridMaterial.uniforms.uColor.value.copy(currentColor);
      gridMaterial.uniforms.uAudioLevel.value = currentAudioLevel;

      // Subtle rotation
      particles.rotation.y = time * 0.03 + xyX * 0.3;

      renderer.render(scene, camera);
    }

    animate();

    function handleResize() {
      camera.aspect = window.innerWidth / window.innerHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(window.innerWidth, window.innerHeight);
    }

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      cancelAnimationFrame(animationId);
      renderer.dispose();
      particleGeometry.dispose();
      particleMaterial.dispose();
      gridGeometry.dispose();
      gridMaterial.dispose();
      if (container && renderer.domElement.parentNode === container) {
        container.removeChild(renderer.domElement);
      }
    };
  });
</script>

<div bind:this={container} class="three-container"></div>

<style>
  .three-container {
    position: fixed;
    top: 0;
    left: 0;
    width: 100%;
    height: 100%;
    z-index: 0;
  }
</style>

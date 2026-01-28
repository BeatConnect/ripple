<script lang="ts">
  import { onMount, onDestroy } from 'svelte';
  import * as THREE from 'three';
  import { visualizerData } from '../stores/params';

  let container: HTMLDivElement;
  let animationId: number;

  onMount(() => {
    // Scene setup
    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
    camera.position.z = 30;

    const renderer = new THREE.WebGLRenderer({ alpha: true, antialias: true });
    renderer.setSize(window.innerWidth, window.innerHeight);
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    container.appendChild(renderer.domElement);

    // Create grid geometry
    const gridSize = 100;
    const divisions = 50;
    const gridGeometry = new THREE.PlaneGeometry(gridSize, gridSize, divisions, divisions);
    const gridMaterial = new THREE.ShaderMaterial({
      uniforms: {
        uTime: { value: 0 },
        uRippleIntensity: { value: 0.5 },
        uColor1: { value: new THREE.Color(0x5eadb0) },  // Cyan
        uColor2: { value: new THREE.Color(0xa855a8) },  // Magenta
        uColor3: { value: new THREE.Color(0x7c5ce0) }   // Purple
      },
      vertexShader: `
        uniform float uTime;
        uniform float uRippleIntensity;

        varying vec2 vUv;
        varying float vElevation;

        void main() {
          vUv = uv;

          vec3 pos = position;

          // Multiple overlapping waves
          float wave1 = sin(pos.x * 0.3 + uTime * 0.5) * cos(pos.y * 0.3 + uTime * 0.3);
          float wave2 = sin(pos.x * 0.5 - uTime * 0.7) * sin(pos.y * 0.4 + uTime * 0.4);
          float wave3 = cos(length(pos.xy) * 0.2 - uTime * 0.6);

          float elevation = (wave1 + wave2 * 0.5 + wave3 * 0.3) * uRippleIntensity * 2.0;
          pos.z += elevation;

          vElevation = elevation;

          gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
        }
      `,
      fragmentShader: `
        uniform vec3 uColor1;
        uniform vec3 uColor2;
        uniform vec3 uColor3;
        uniform float uTime;

        varying vec2 vUv;
        varying float vElevation;

        void main() {
          // Mix colors based on elevation and position
          float mixFactor1 = smoothstep(-1.0, 1.0, vElevation);
          float mixFactor2 = sin(vUv.x * 3.14159 + uTime * 0.2) * 0.5 + 0.5;

          vec3 color = mix(uColor1, uColor2, mixFactor1);
          color = mix(color, uColor3, mixFactor2 * 0.3);

          // Grid lines
          vec2 grid = abs(fract(vUv * 50.0 - 0.5) - 0.5);
          float line = min(grid.x, grid.y);
          float lineIntensity = 1.0 - smoothstep(0.0, 0.02, line);

          // Apply color with line intensity
          color += lineIntensity * 0.3;

          // Fade edges
          float fade = smoothstep(0.0, 0.3, vUv.x) * smoothstep(1.0, 0.7, vUv.x);
          fade *= smoothstep(0.0, 0.3, vUv.y) * smoothstep(1.0, 0.7, vUv.y);

          gl_FragColor = vec4(color, fade * 0.6);
        }
      `,
      transparent: true,
      side: THREE.DoubleSide,
      wireframe: false
    });

    const grid = new THREE.Mesh(gridGeometry, gridMaterial);
    grid.rotation.x = -Math.PI * 0.5;
    grid.position.y = -15;
    scene.add(grid);

    // Add floating particles
    const particleCount = 200;
    const particleGeometry = new THREE.BufferGeometry();
    const positions = new Float32Array(particleCount * 3);
    const sizes = new Float32Array(particleCount);

    for (let i = 0; i < particleCount; i++) {
      positions[i * 3] = (Math.random() - 0.5) * 80;
      positions[i * 3 + 1] = (Math.random() - 0.5) * 40;
      positions[i * 3 + 2] = (Math.random() - 0.5) * 40;
      sizes[i] = Math.random() * 2 + 0.5;
    }

    particleGeometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
    particleGeometry.setAttribute('size', new THREE.BufferAttribute(sizes, 1));

    const particleMaterial = new THREE.ShaderMaterial({
      uniforms: {
        uTime: { value: 0 },
        uColor: { value: new THREE.Color(0x5eadb0) }
      },
      vertexShader: `
        uniform float uTime;
        attribute float size;

        void main() {
          vec3 pos = position;
          pos.y += sin(uTime * 0.3 + position.x * 0.1) * 2.0;
          pos.x += cos(uTime * 0.2 + position.z * 0.1) * 1.0;

          gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
          gl_PointSize = size * (30.0 / gl_Position.w);
        }
      `,
      fragmentShader: `
        uniform vec3 uColor;

        void main() {
          float dist = length(gl_PointCoord - 0.5);
          if (dist > 0.5) discard;

          float alpha = 1.0 - smoothstep(0.2, 0.5, dist);
          gl_FragColor = vec4(uColor, alpha * 0.4);
        }
      `,
      transparent: true,
      depthWrite: false
    });

    const particles = new THREE.Points(particleGeometry, particleMaterial);
    scene.add(particles);

    // Animation
    let time = 0;
    let rippleIntensity = 0.5;

    const unsubscribe = visualizerData.subscribe((data) => {
      // Use LFO values to modulate the background
      const avgLfo = data.lfoValues.reduce((a, b) => a + b, 0) / data.lfoValues.length;
      rippleIntensity = 0.3 + avgLfo * 0.7;
    });

    function animate() {
      time += 0.016;

      gridMaterial.uniforms.uTime.value = time;
      gridMaterial.uniforms.uRippleIntensity.value = rippleIntensity;
      particleMaterial.uniforms.uTime.value = time;

      // Subtle camera movement
      camera.position.x = Math.sin(time * 0.1) * 3;
      camera.position.y = Math.cos(time * 0.08) * 2 + 5;
      camera.lookAt(0, -5, 0);

      renderer.render(scene, camera);
      animationId = requestAnimationFrame(animate);
    }

    animate();

    // Handle resize
    function handleResize() {
      camera.aspect = window.innerWidth / window.innerHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(window.innerWidth, window.innerHeight);
    }

    window.addEventListener('resize', handleResize);

    return () => {
      window.removeEventListener('resize', handleResize);
      cancelAnimationFrame(animationId);
      unsubscribe();
      renderer.dispose();
      gridGeometry.dispose();
      gridMaterial.dispose();
      particleGeometry.dispose();
      particleMaterial.dispose();
      container.removeChild(renderer.domElement);
    };
  });
</script>

<div bind:this={container} id="three-canvas"></div>

/*
  ==============================================================================
    RIPPLE - Spectral Processor
    FFT-based spectral freeze and smear using proper STFT overlap-add
    Based on: https://audiodev.blog/fft-processing/
  ==============================================================================
*/

#include "SpectralProcessor.h"
#include <cmath>
#include <cstring>

SpectralProcessor::SpectralProcessor()
{
    for (int i = 0; i < NUM_BINS; ++i)
    {
        visualMagnitude[i].store(0.0f);
        visualFrozen[i].store(0.0f);
    }
    reset();
}

void SpectralProcessor::prepare(double newSampleRate, int)
{
    sampleRate = newSampleRate;
    reset();
}

void SpectralProcessor::reset()
{
    inputFifo.fill(0.0f);
    outputFifo.fill(0.0f);
    fftData.fill(0.0f);
    magnitude.fill(0.0f);
    phase.fill(0.0f);
    frozenMagnitude.fill(0.0f);
    frozenPhase.fill(0.0f);
    smearBuffer.fill(0.0f);
    feedbackBuffer.fill(0.0f);
    shiftedMagnitude.fill(0.0f);

    fifoPos = 0;
    frameCount = 0;
}

void SpectralProcessor::processFrame()
{
    float* fftPtr = fftData.data();
    const float* inputPtr = inputFifo.data();

    // Step 1: Copy circular FIFO to FFT buffer (unwrap the ring buffer)
    std::memcpy(fftPtr, inputPtr + fifoPos, (FFT_SIZE - fifoPos) * sizeof(float));
    if (fifoPos > 0)
    {
        std::memcpy(fftPtr + FFT_SIZE - fifoPos, inputPtr, fifoPos * sizeof(float));
    }

    // Step 2: Apply window before FFT
    window.multiplyWithWindowingTable(fftPtr, FFT_SIZE);

    // Step 3: Forward FFT
    fft.performRealOnlyForwardTransform(fftPtr, true);

    // Step 4: Process spectrum (extract magnitudes for visualization, apply effects)
    processSpectrum();

    // Step 5: Inverse FFT
    fft.performRealOnlyInverseTransform(fftPtr);

    // Step 6: Apply window again and gain correction
    window.multiplyWithWindowingTable(fftPtr, FFT_SIZE);

    // Gain correction: 2/3 for Hann window squared, divided by overlap factor
    const float gain = windowCorrection;
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        fftPtr[i] *= gain;
    }

    // Step 7: Overlap-add to output FIFO
    // The processed frame needs to be added at the correct position
    for (int i = 0; i < FFT_SIZE; ++i)
    {
        int outIdx = (fifoPos + i) % FFT_SIZE;
        outputFifo[outIdx] += fftPtr[i];
    }
}

void SpectralProcessor::processSpectrum()
{
    float* fftPtr = fftData.data();

    // Get parameters
    const float freeze = freezeAmount.load();
    const float smear = smearAmount.load();
    const float scatter = scatterAmount.load();
    const float shift = shiftAmount.load();
    const float tilt = tiltAmount.load();
    const float feedback = feedbackAmount.load();

    // Temporary arrays for spectral processing
    std::array<float, NUM_BINS> tempMag;
    std::array<float, NUM_BINS> tempPhase;

    // First pass: extract all magnitudes and phases
    for (int i = 0; i < NUM_BINS; ++i)
    {
        float real = fftPtr[i * 2];
        float imag = fftPtr[i * 2 + 1];
        tempMag[i] = std::sqrt(real * real + imag * imag);
        tempPhase[i] = std::atan2(imag, real);
    }

    // === SHIFT EFFECT (spectral pitch shift) ===
    if (std::abs(shift) > 0.01f)
    {
        shiftedMagnitude.fill(0.0f);

        // Calculate shift amount in bins (shift of 1 = one octave up)
        float shiftRatio = std::pow(2.0f, shift);

        for (int i = 1; i < NUM_BINS - 1; ++i)
        {
            int newBin = static_cast<int>(i * shiftRatio);
            if (newBin > 0 && newBin < NUM_BINS - 1)
            {
                shiftedMagnitude[newBin] += tempMag[i];
            }
        }

        // Blend original with shifted
        float shiftBlend = std::abs(shift);
        for (int i = 0; i < NUM_BINS; ++i)
        {
            tempMag[i] = tempMag[i] * (1.0f - shiftBlend) + shiftedMagnitude[i] * shiftBlend;
        }
    }

    // === TILT EFFECT (spectral EQ - dark to bright) ===
    if (std::abs(tilt) > 0.01f)
    {
        for (int i = 1; i < NUM_BINS; ++i)
        {
            // Calculate tilt factor based on frequency position
            float freqNorm = static_cast<float>(i) / NUM_BINS;
            float tiltGain;

            if (tilt > 0)
            {
                // Positive tilt: boost highs, cut lows
                tiltGain = 1.0f + tilt * (freqNorm * 2.0f - 1.0f);
            }
            else
            {
                // Negative tilt: boost lows, cut highs
                tiltGain = 1.0f - tilt * (1.0f - freqNorm * 2.0f);
            }

            tiltGain = juce::jlimit(0.2f, 3.0f, tiltGain);
            tempMag[i] *= tiltGain;
        }
    }

    // Process each bin
    for (int i = 0; i < NUM_BINS; ++i)
    {
        float mag = tempMag[i];
        float ph = tempPhase[i];

        // === FEEDBACK (self-modulation for evolving textures) ===
        if (feedback > 0.01f)
        {
            mag += feedbackBuffer[i] * feedback * 0.8f;
            feedbackBuffer[i] = feedbackBuffer[i] * 0.95f + mag * 0.05f;
        }
        else
        {
            feedbackBuffer[i] *= 0.9f;
        }

        // === FREEZE EFFECT ===
        if (freeze > 0.01f)
        {
            float captureRate = 0.05f * (1.0f - freeze * 0.95f);
            frozenMagnitude[i] = frozenMagnitude[i] * (1.0f - captureRate) + mag * captureRate;
            frozenPhase[i] = ph;
            mag = mag * (1.0f - freeze) + frozenMagnitude[i] * freeze;
        }

        // === SMEAR/SUSTAIN EFFECT ===
        if (smear > 0.01f)
        {
            float decayRate = 0.85f + smear * 0.145f;  // 0.85 to 0.995
            smearBuffer[i] = juce::jmax(smearBuffer[i] * decayRate, mag);
            mag = mag * (1.0f - smear * 0.9f) + smearBuffer[i] * smear * 0.9f;
        }
        else
        {
            smearBuffer[i] *= 0.8f;
        }

        // === SCATTER/DIFFUSE EFFECT ===
        if (scatter > 0.01f)
        {
            float phaseNoise = (random.nextFloat() * 2.0f - 1.0f) * juce::MathConstants<float>::pi;
            ph += phaseNoise * scatter;

            if (i > 1 && i < NUM_BINS - 2)
            {
                float blurAmount = scatter * 0.5f;
                float neighborAvg = (tempMag[i-2] + tempMag[i-1] + tempMag[i+1] + tempMag[i+2]) * 0.25f;
                mag = mag * (1.0f - blurAmount) + neighborAvg * blurAmount;
            }
        }

        // Store magnitude for visualization
        magnitude[i] = mag;

        // Update visualization
        float displayMag = mag / static_cast<float>(FFT_SIZE);
        displayMag = std::pow(displayMag, 0.35f);
        visualMagnitude[i].store(juce::jlimit(0.0f, 1.0f, displayMag * 10.0f));

        float displayFrozen = frozenMagnitude[i] / static_cast<float>(FFT_SIZE);
        displayFrozen = std::pow(displayFrozen, 0.35f);
        visualFrozen[i].store(juce::jlimit(0.0f, 1.0f, displayFrozen * 10.0f));

        // Reconstruct complex value
        fftPtr[i * 2] = mag * std::cos(ph);
        fftPtr[i * 2 + 1] = mag * std::sin(ph);
    }
}

void SpectralProcessor::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Sum channels to mono for input
        float monoIn = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            monoIn += buffer.getSample(ch, sample);
        }
        monoIn /= static_cast<float>(numChannels);

        // Push sample into input FIFO
        inputFifo[fifoPos] = monoIn;

        // Read from output FIFO
        float monoOut = outputFifo[fifoPos];
        outputFifo[fifoPos] = 0.0f;  // Clear for next overlap-add

        // Write to all output channels
        for (int ch = 0; ch < numChannels; ++ch)
        {
            buffer.setSample(ch, sample, monoOut);
        }

        // Advance position
        fifoPos = (fifoPos + 1) % FFT_SIZE;
        frameCount++;

        // Process frame every hopSize samples
        if (frameCount >= HOP_SIZE)
        {
            frameCount = 0;
            processFrame();
        }
    }
}

void SpectralProcessor::getMagnitudeSpectrum(float* magnitudes, int numBins) const
{
    int count = juce::jmin(numBins, NUM_BINS);
    for (int i = 0; i < count; ++i)
    {
        magnitudes[i] = visualMagnitude[i].load();
    }
}

void SpectralProcessor::getFrozenSpectrum(float* magnitudes, int numBins) const
{
    int count = juce::jmin(numBins, NUM_BINS);
    for (int i = 0; i < count; ++i)
    {
        magnitudes[i] = visualFrozen[i].load();
    }
}

/*
  ==============================================================================
    RIPPLE - Spectral Processor
    FFT-based spectral freeze, smear, and manipulation
    Based on proper STFT overlap-add implementation
  ==============================================================================
*/

#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

class SpectralProcessor
{
public:
    static constexpr int FFT_ORDER = 10;  // 1024 samples - lower latency
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;
    static constexpr int NUM_BINS = FFT_SIZE / 2 + 1;
    static constexpr int OVERLAP = 4;  // 75% overlap
    static constexpr int HOP_SIZE = FFT_SIZE / OVERLAP;

    SpectralProcessor();

    void prepare(double sampleRate, int maxBlockSize);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    // Control parameters
    void setFreezeAmount(float amount) { freezeAmount.store(amount); }
    void setSmearAmount(float amount) { smearAmount.store(amount); }
    void setScatterAmount(float amount) { scatterAmount.store(amount); }
    void setShiftAmount(float amount) { shiftAmount.store(amount); }      // -1 to +1
    void setTiltAmount(float amount) { tiltAmount.store(amount); }        // -1 to +1
    void setFeedbackAmount(float amount) { feedbackAmount.store(amount); } // 0 to 1

    // Interaction zones (normalized 0-1)
    void setInteractionY(float y) { interactionY.store(y); }
    void setInteractionRadius(float r) { interactionRadius.store(r); }
    void setInteractionActive(bool active) { interactionActive.store(active); }

    // Get magnitude spectrum for visualization
    void getMagnitudeSpectrum(float* magnitudes, int numBins) const;
    void getFrozenSpectrum(float* magnitudes, int numBins) const;

private:
    void processFrame();
    void processSpectrum();

    juce::dsp::FFT fft { FFT_ORDER };
    juce::dsp::WindowingFunction<float> window { FFT_SIZE + 1,
        juce::dsp::WindowingFunction<float>::hann, false };

    // Gain correction for Hann window with 75% overlap
    // Squared Hann has avg value 3/8, times 4 overlaps = 1.5, so multiply by 2/3
    static constexpr float windowCorrection = 2.0f / 3.0f;

    // FIFO buffers
    std::array<float, FFT_SIZE> inputFifo;
    std::array<float, FFT_SIZE> outputFifo;
    int fifoPos = 0;
    int frameCount = 0;

    // FFT working data
    std::array<float, FFT_SIZE * 2> fftData;

    // Spectral data
    std::array<float, NUM_BINS> magnitude;
    std::array<float, NUM_BINS> phase;
    std::array<float, NUM_BINS> frozenMagnitude;
    std::array<float, NUM_BINS> frozenPhase;
    std::array<float, NUM_BINS> smearBuffer;
    std::array<float, NUM_BINS> feedbackBuffer;
    std::array<float, NUM_BINS> shiftedMagnitude;

    // Parameters
    std::atomic<float> freezeAmount { 0.0f };
    std::atomic<float> smearAmount { 0.0f };
    std::atomic<float> scatterAmount { 0.0f };
    std::atomic<float> shiftAmount { 0.0f };
    std::atomic<float> tiltAmount { 0.0f };
    std::atomic<float> feedbackAmount { 0.0f };
    std::atomic<float> interactionY { 0.5f };
    std::atomic<float> interactionRadius { 0.2f };
    std::atomic<bool> interactionActive { false };

    double sampleRate = 44100.0;

    // Visualization (atomic for thread safety)
    mutable std::array<std::atomic<float>, NUM_BINS> visualMagnitude;
    mutable std::array<std::atomic<float>, NUM_BINS> visualFrozen;

    juce::Random random;
};

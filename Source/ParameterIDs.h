/*
  ==============================================================================
    RIPPLE Texture Engine - Parameter IDs
    Transform any sound into evolving hypnotic textures

    Define all parameter identifiers as constexpr strings.
    These IDs must match exactly between:
    - C++ (APVTS parameter creation)
    - C++ (relay creation)
    - TypeScript/Svelte (getSliderState(), etc.)
  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    // ===========================================================================
    // Ripple Filter Parameters (Core)
    // ===========================================================================
    inline constexpr const char* rippleRate     = "ripple_rate";      // Wave speed (0.01-20 Hz)
    inline constexpr const char* rippleMultiply = "ripple_multiply";  // Wave intensity
    inline constexpr const char* rippleAmount   = "ripple_amount";    // Ripples within ripples
    inline constexpr const char* rippleWidth    = "ripple_width";     // Stereo width
    inline constexpr const char* rippleLowBypass  = "ripple_low_bypass";   // Low freq bypass
    inline constexpr const char* rippleHighBypass = "ripple_high_bypass";  // High freq bypass
    inline constexpr const char* rippleMix      = "ripple_mix";       // Dry/Wet mix

    // ===========================================================================
    // Reverb Parameters
    // ===========================================================================
    inline constexpr const char* reverbEnabled = "reverb_enabled";
    inline constexpr const char* reverbSize    = "reverb_size";
    inline constexpr const char* reverbDamping = "reverb_damping";
    inline constexpr const char* reverbMix     = "reverb_mix";

    // ===========================================================================
    // LFO Parameters (4 LFOs)
    // ===========================================================================
    // LFO 1
    inline constexpr const char* lfo1Rate  = "lfo1_rate";
    inline constexpr const char* lfo1Shape = "lfo1_shape";
    inline constexpr const char* lfo1Phase = "lfo1_phase";

    // LFO 2
    inline constexpr const char* lfo2Rate  = "lfo2_rate";
    inline constexpr const char* lfo2Shape = "lfo2_shape";
    inline constexpr const char* lfo2Phase = "lfo2_phase";

    // LFO 3
    inline constexpr const char* lfo3Rate  = "lfo3_rate";
    inline constexpr const char* lfo3Shape = "lfo3_shape";
    inline constexpr const char* lfo3Phase = "lfo3_phase";

    // LFO 4
    inline constexpr const char* lfo4Rate  = "lfo4_rate";
    inline constexpr const char* lfo4Shape = "lfo4_shape";
    inline constexpr const char* lfo4Phase = "lfo4_phase";

    // ===========================================================================
    // Modulation Matrix (4 Slots)
    // ===========================================================================
    // Mod Slot 1
    inline constexpr const char* mod1Source = "mod1_source";
    inline constexpr const char* mod1Target = "mod1_target";
    inline constexpr const char* mod1Depth  = "mod1_depth";

    // Mod Slot 2
    inline constexpr const char* mod2Source = "mod2_source";
    inline constexpr const char* mod2Target = "mod2_target";
    inline constexpr const char* mod2Depth  = "mod2_depth";

    // Mod Slot 3
    inline constexpr const char* mod3Source = "mod3_source";
    inline constexpr const char* mod3Target = "mod3_target";
    inline constexpr const char* mod3Depth  = "mod3_depth";

    // Mod Slot 4
    inline constexpr const char* mod4Source = "mod4_source";
    inline constexpr const char* mod4Target = "mod4_target";
    inline constexpr const char* mod4Depth  = "mod4_depth";

    // ===========================================================================
    // Global Parameters
    // ===========================================================================
    inline constexpr const char* bypass      = "bypass";
    inline constexpr const char* inputGain   = "input_gain";
    inline constexpr const char* outputGain  = "output_gain";
    inline constexpr const char* randomSeed  = "random_seed";  // For reproducible randomization
}

// ===========================================================================
// LFO Shape Options
// ===========================================================================
namespace LFOShapes
{
    inline const juce::StringArray shapes {
        "Sine",
        "Triangle",
        "Square",
        "Saw Up",
        "Saw Down",
        "Random"
    };
}

// ===========================================================================
// Modulation Sources
// ===========================================================================
namespace ModSources
{
    inline const juce::StringArray sources {
        "None",
        "LFO 1",
        "LFO 2",
        "LFO 3",
        "LFO 4"
    };
}

// ===========================================================================
// Modulation Targets
// ===========================================================================
namespace ModTargets
{
    inline const juce::StringArray targets {
        "None",
        "Ripple Rate",
        "Ripple Multiply",
        "Ripple Amount",
        "Ripple Width",
        "Ripple Low Bypass",
        "Ripple High Bypass",
        "Ripple Mix",
        "Reverb Size",
        "Reverb Damping",
        "Reverb Mix"
    };
}

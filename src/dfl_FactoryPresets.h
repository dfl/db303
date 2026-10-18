#pragma once

#include <JuceHeader.h>

#include <optional>
#include <utility>
#include <vector>

namespace dfl
{

// A factory sound, loaded from presets/*.json (built into the plugin, ordered by file name):
// parameter ID -> value: a choice name (or index), true / false for switches, the raw value for floats.
// Every sound parameter not listed goes back to its default, so Init is empty.
// knob1 / knob2 are the parameters on the two performance knobs; the mod wheel drives knob1.
struct FactoryPreset
{
    juce::String name;
    juce::String description;
    std::vector<std::pair<juce::String, juce::var>> values;
    juce::String knob1 = "cutoff";
    juce::String knob2 = "lfoDepth";
};

/** Parses one preset file: { "name", "description", "knobs": [k1, k2], "values": { id: value }, "notes": { id: text } }. */
FactoryPreset parseFactoryPreset (const juce::String& json);

const std::vector<FactoryPreset>& factoryPresets();

/** A preset value in the parameter's own units, or nothing if it does not fit the parameter
    (an unknown choice name, a name on a numeric parameter). */
inline std::optional<float> resolvePresetValue (const juce::RangedAudioParameter& param, const juce::var& v)
{
    if (v.isString())
    {
        if (auto* choice = dynamic_cast<const juce::AudioParameterChoice*> (&param))
            if (const int index = choice->choices.indexOf (v.toString()); index >= 0)
                return (float) index;
        return std::nullopt;
    }
    if (v.isBool())
        return (bool) v ? 1.0f : 0.0f;
    if (v.isInt() || v.isInt64() || v.isDouble())
        return (float) (double) v;
    return std::nullopt;
}

// the performance knob assignments, kept as properties of the plugin state
inline const juce::Identifier performanceKnobIds[2] { "perfKnob1", "perfKnob2" };
inline const char* const defaultPerformanceKnobs[2] { "cutoff", "lfoDepth" };

// Presets are sounds: the sequencer, the generator and the output level and tuning stay as they are.
inline bool isPresetParameter (const juce::String& id)
{
    return ! id.startsWith ("seq") && id != "numberOfTones" && id != "lowerNote" && id != "rangeNote"
        && id != "volume" && id != "tuning";
}

inline void applyFactoryPreset (const juce::Array<juce::AudioProcessorParameter*>& parameters, const FactoryPreset& preset)
{
    for (auto* p : parameters)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (param == nullptr || ! isPresetParameter (param->getParameterID()))
            continue;

        float normalised = param->getDefaultValue();
        for (auto& [id, v] : preset.values)
            if (param->getParameterID() == id)
                if (const auto value = resolvePresetValue (*param, v))
                    normalised = param->convertTo0to1 (*value);

        param->beginChangeGesture();
        param->setValueNotifyingHost (normalised);
        param->endChangeGesture();
    }
}

} // namespace dfl

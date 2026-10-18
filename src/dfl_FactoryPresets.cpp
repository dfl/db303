#include "dfl_FactoryPresets.h"
#include "FactoryPresetData.h"

#include <algorithm>

namespace dfl
{

FactoryPreset parseFactoryPreset (const juce::String& json)
{
    const auto root = juce::JSON::parse (json);
    FactoryPreset preset;
    preset.name = root["name"].toString();
    preset.description = root["description"].toString();

    if (auto* knobs = root["knobs"].getArray(); knobs != nullptr && knobs->size() == 2)
    {
        preset.knob1 = (*knobs)[0].toString();
        preset.knob2 = (*knobs)[1].toString();
    }

    if (auto* values = root["values"].getDynamicObject())
        for (auto& [id, v] : values->getProperties())
            preset.values.emplace_back (id.toString(), v);

    return preset;
}

const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> presets = []
    {
        std::vector<std::pair<juce::String, const char*>> files;   // original file name -> resource name
        for (int i = 0; i < FactoryPresetData::namedResourceListSize; ++i)
            files.emplace_back (FactoryPresetData::originalFilenames[i], FactoryPresetData::namedResourceList[i]);
        std::sort (files.begin(), files.end());

        std::vector<FactoryPreset> list;
        for (auto& [file, resource] : files)
        {
            int size = 0;
            const char* data = FactoryPresetData::getNamedResource (resource, size);
            list.push_back (parseFactoryPreset (juce::String::fromUTF8 (data, size)));
        }
        return list;
    }();
    return presets;
}

} // namespace dfl

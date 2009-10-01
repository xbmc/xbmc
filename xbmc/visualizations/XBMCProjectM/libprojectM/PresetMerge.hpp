#ifndef PRESET_MERGE_HPP
#define PRESET_MERGE_HPP
#include "Preset.hpp"

class PresetMerger 
{
public:
  static void MergePresets(PresetOutputs & A,  PresetOutputs & B, double ratio, int gx, int gy);
};

#endif

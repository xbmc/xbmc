#ifndef IDLE_PRESET_HPP
#define IDLE_PRESET_HPP
#include <memory>
#include "PresetFrameIO.hpp"
#include "Preset.hpp"

/// A preset that does not depend on the file system to be loaded. This allows projectM to render
/// something (ie. self indulgent project advertisting) even when no valid preset directory is found.
class IdlePreset {

  public:
	/// Allocate a new idle preset instance 
	/// \param presetInputs the preset inputs instance to associate with the preset
	/// \param presetOutputs the preset output instance to associate with the preset
	/// \returns a newly allocated auto pointer of an idle preset instance
	static std::unique_ptr<Preset> allocate(PresetInputs & presetInputs, PresetOutputs & presetOutputs);
  private:
	static std::string presetText();
	static const std::string IDLE_PRESET_NAME;
};
#endif

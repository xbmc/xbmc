#ifndef IDLE_PRESET_HPP
#define IDLE_PRESET_HPP
#include <memory>
#include <string>

class PresetOutputs;
class Preset;
/// A preset that does not depend on the file system to be loaded. This allows projectM to render
/// something (ie. self indulgent project advertising) even when no valid preset directory is found.
class IdlePresets {

  public:
	/// Allocate a new idle preset instance
	/// \returns a newly allocated auto pointer of an idle preset instance
	static std::auto_ptr<Preset> allocate(const std::string & path, PresetOutputs & outputs);
  private:
	static std::string presetText();
	static const std::string IDLE_PRESET_NAME;
};
#endif

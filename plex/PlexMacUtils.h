#pragma once

#include <string>
#include <vector>

#ifdef TARGET_DARWIN_OSX

namespace PlexMacUtils {
  std::vector<std::string> GetSystemFonts();
  std::string GetSystemFontPathFromDisplayName(const std::string displayName);
}

#endif

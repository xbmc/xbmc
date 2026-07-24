/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class TiXmlElement;

namespace KODI::GUILIB
{

/*!
 \brief Manages skin-defined label maps loaded from an includes file (e.g. Includes_Maps.xml).

 Skin authors define \<map\> elements containing \<entry key="rawValue"\>Display Value\</entry\>
 children. At runtime, $MAP[MapName, infolabel] resolves the infolabel to its current string
 value and looks it up in the named map. If the key is not found, the raw infolabel value is
 returned unchanged.

 Maps support a \p ref attribute to alias one map to another, avoiding duplicated entry data.
 A map with both \p ref and \<entry\> children inherits from the referenced map but overrides
 specific keys.

 Example XML:
 \code{.xml}
 <map name="DefaultCodecMap">
   <entry key="ac3">Dolby Digital</entry>
   <entry key="eac3">Dolby Digital+</entry>
 </map>

 <map name="DefaultAltAudioMap" ref="DefaultCodecMap">
   <entry key="eac3_ddp_atmos">Dolby Digital+ / Atmos</entry>
 </map>
 \endcode

 \since v22
*/
class CSkinMapManager
{
public:
  CSkinMapManager() = default;
  ~CSkinMapManager() = default;

  struct StringHash
  {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept
    {
      return std::hash<std::string_view>{}(sv);
    }
  };

  using SkinMap = std::unordered_map<std::string, std::string, StringHash, std::equal_to<>>;

  /*! \brief Clear all loaded maps */
  void Clear();

  /*! \brief Load all \<map\> elements from the given XML node (root of an includes file).
   Handles both standard maps with \<entry\> children and maps with a \p ref attribute that
   alias another map, optionally with override entries.
   \param node the root element of an includes file
  */
  void LoadMaps(const TiXmlElement* node);

  /*! \brief Look up a value in the named map, following ref chains recursively.
   \param mapName  the skin-defined map to search
   \param key      the raw infolabel value to look up
   \param visited  chain of already-visited map names, used for cycle detection
   \return the mapped display string, or \p key unchanged if no mapping is found
  */
  std::string Lookup(std::string_view mapName,
                     std::string_view key,
                     std::vector<std::string> visited = {}) const;

private:
  using SkinMapsMap = std::unordered_map<std::string, SkinMap, StringHash, std::equal_to<>>;
  using RefMap = SkinMap;

  SkinMapsMap m_maps;
  RefMap m_refs;
};

} // namespace KODI::GUILIB

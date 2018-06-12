/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "addons/Resource.h"
#include <memory>

namespace ADDON
{
class CFontResource : public CResource
{
public:
  static std::unique_ptr<CFontResource> FromExtension(CAddonInfo addonInfo,
                                                      const cp_extension_t* ext);

  explicit CFontResource(CAddonInfo addonInfo) : CResource(std::move(addonInfo)) {}

  //! \brief Check whether file is allowed or not (no filters here).
  bool IsAllowed(const std::string& file) const override { return true; }

  //! \brief Get the font path if given font file is served by the add-on.
  //! \param[in] file File name of font.
  //! \param[out] path Full path to font if found.
  //! \return True if font was found, false otherwise.
  bool GetFont(const std::string& file, std::string& path) const;

  //! \brief Callback executed after installation
  void OnPostInstall(bool update, bool modal) override;
};

}

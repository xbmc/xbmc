#pragma once
/*
 *      Copyright (C) 2015-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "addons/Resource.h"

namespace ADDON
{

class CUISoundsResource : public CResource
{
public:
  explicit CUISoundsResource(CAddonInfo addonInfo) : CResource(std::move(addonInfo)) {}
  bool IsAllowed(const std::string &file) const override;
  bool IsInUse() const override;
  void OnPostInstall(bool update, bool modal) override;
};

}

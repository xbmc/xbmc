/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "addons/Resource.h"

#include <memory>

namespace ADDON
{

class CGameResource : public CResource
{
public:
  CGameResource(const AddonInfoPtr& addonInfo);
  virtual ~CGameResource() = default;

  static std::unique_ptr<CGameResource> FromExtension(const AddonInfoPtr& addonInfo, const cp_extension_t* ext);

  // implementation of CResource
  virtual bool IsAllowed(const std::string& file) const override { return true; }
};

}

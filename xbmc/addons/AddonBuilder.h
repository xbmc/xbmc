#pragma once
/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include "Addon.h"
#include "Service.h"

namespace ADDON
{

class CAddonBuilder
{
public:
  CAddonBuilder() : m_built(false) {}

  std::shared_ptr<CAddon> Build();
  void SetId(std::string id) { m_addonInfo.m_id = std::move(id); }
  void SetPath(std::string path) { m_addonInfo.m_path = std::move(path); }

  const std::string& GetId() const { return m_addonInfo.m_id; }

private:
  bool m_built;
  CAddonInfo m_addonInfo;
};

};
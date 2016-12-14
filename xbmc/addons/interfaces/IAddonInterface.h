#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

#ifdef _WIN32
#include <windows.h>
#define A_DLLEXPORT extern "C" __declspec(dllexport)
#else
#define A_DLLEXPORT extern "C" __attribute__ ((visibility ("default")))
#endif

namespace ADDON
{
  class CAddon;

  class IAddonInterface
  {
  public:
    IAddonInterface(CAddon *addon, int apiLevel, const std::string& version) :
      m_addon(addon), m_apiLevel(apiLevel), m_version(version)  {}

    CAddon*            GetAddon()       { return m_addon;    }
    const CAddon*      GetAddon() const { return m_addon;    }
    const int          APILevel() const { return m_apiLevel; }
    const std::string& Version()  const { return m_version;  }

  protected:
    CAddon*           m_addon;     /*!< the addon */
    const int         m_apiLevel;
    const std::string m_version;
  };

} /* namespace ADDON */

/*
*      Copyright (C) 2015 Team XBMC
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
#include "UISoundsResource.h"
#include "guilib/GUIAudioManager.h"
#include "settings/Settings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"


namespace ADDON
{

bool CUISoundsResource::IsAllowed(const std::string& file) const
{
  return StringUtils::EqualsNoCase(file, "sounds.xml")
      || URIUtils::HasExtension(file, ".wav");
}

bool CUISoundsResource::IsInUse() const
{
  return CSettings::GetInstance().GetString(CSettings::SETTING_LOOKANDFEEL_SOUNDSKIN) == ID();
}

void CUISoundsResource::OnPostInstall(bool update, bool modal)
{
  if (IsInUse())
    g_audioManager.Load();
}

}

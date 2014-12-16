/*
 *      Copyright (C) 2005-2013 Team XBMC
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
#include "PluginSource.h"
#include "AddonManager.h"
#include "utils/StringUtils.h"

using namespace std;

namespace ADDON
{

CPluginSource::CPluginSource(const AddonProps &props)
  : CAddon(props)
{
  std::string provides;
  InfoMap::const_iterator i = Props().extrainfo.find("provides");
  if (i != Props().extrainfo.end())
    provides = i->second;
  SetProvides(provides);
}

CPluginSource::CPluginSource(const cp_extension_t *ext)
  : CAddon(ext)
{
  std::string provides;
  if (ext)
  {
    provides = CAddonMgr::Get().GetExtValue(ext->configuration, "provides");
    if (!provides.empty())
      Props().extrainfo.insert(make_pair("provides", provides));
  }
  SetProvides(provides);
}

AddonPtr CPluginSource::Clone() const
{
  return AddonPtr(new CPluginSource(*this));
}

void CPluginSource::SetProvides(const std::string &content)
{
  if (!content.empty())
  {
    vector<string> provides = StringUtils::Split(content, ' ');
    for (vector<string>::const_iterator i = provides.begin(); i != provides.end(); ++i)
    {
      Content content = Translate(*i);
      if (content != UNKNOWN)
        m_providedContent.insert(content);
    }
  }
  if (Type() == ADDON_SCRIPT && m_providedContent.empty())
    m_providedContent.insert(EXECUTABLE);
}

CPluginSource::Content CPluginSource::Translate(const std::string &content)
{
  if (content == "audio")
    return CPluginSource::AUDIO;
  else if (content == "image")
    return CPluginSource::IMAGE;
  else if (content == "executable")
    return CPluginSource::EXECUTABLE;
  else if (content == "video")
    return CPluginSource::VIDEO;
  else
    return CPluginSource::UNKNOWN;
}

bool CPluginSource::IsType(TYPE type) const
{
  return ((type == ADDON_VIDEO && Provides(VIDEO))
       || (type == ADDON_AUDIO && Provides(AUDIO))
       || (type == ADDON_IMAGE && Provides(IMAGE))
       || (type == ADDON_EXECUTABLE && Provides(EXECUTABLE)));
}

} /*namespace ADDON*/


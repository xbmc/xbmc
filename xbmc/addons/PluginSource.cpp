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

#include <utility>

#include "AddonManager.h"
#include "utils/StringUtils.h"

namespace ADDON
{

std::unique_ptr<CPluginSource> CPluginSource::FromExtension(AddonProps props, const cp_extension_t* ext)
{
  std::string provides = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "provides");
  if (!provides.empty())
    props.extrainfo.insert(make_pair("provides", provides));
  return std::unique_ptr<CPluginSource>(new CPluginSource(std::move(props), provides));
}

CPluginSource::CPluginSource(AddonProps props) : CAddon(std::move(props))
{
  std::string provides;
  InfoMap::const_iterator i = m_props.extrainfo.find("provides");
  if (i != m_props.extrainfo.end())
    provides = i->second;
  SetProvides(provides);
}

CPluginSource::CPluginSource(AddonProps props, const std::string& provides)
  : CAddon(std::move(props))
{
  SetProvides(provides);
}

void CPluginSource::SetProvides(const std::string &content)
{
  if (!content.empty())
  {
    std::vector<std::string> provides = StringUtils::Split(content, ' ');
    for (std::vector<std::string>::const_iterator i = provides.begin(); i != provides.end(); ++i)
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

TYPE CPluginSource::FullType() const
{
  if (Provides(VIDEO))
    return ADDON_VIDEO;
  if (Provides(AUDIO))
    return ADDON_AUDIO;
  if (Provides(IMAGE))
    return ADDON_IMAGE;
  if (Provides(EXECUTABLE))
    return ADDON_EXECUTABLE;

  return CAddon::FullType();
}

bool CPluginSource::IsType(TYPE type) const
{
  return ((type == ADDON_VIDEO && Provides(VIDEO))
       || (type == ADDON_AUDIO && Provides(AUDIO))
       || (type == ADDON_IMAGE && Provides(IMAGE))
       || (type == ADDON_EXECUTABLE && Provides(EXECUTABLE)));
}

} /*namespace ADDON*/


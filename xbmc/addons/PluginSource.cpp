/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#include "PluginSource.h"
#include "AddonManager.h"

namespace ADDON
{

CPluginSource::CPluginSource(cp_plugin_info_t *props, const std::set<CPluginSource::Content> &provides)
  : CAddon(props)
  , m_providedContent(provides)
{
}

CPluginSource::Content CPluginSource::Translate(const CStdString &content)
{
  if (content.Equals("audio"))
    return CPluginSource::AUDIO;
  else if (content.Equals("image"))
    return CPluginSource::IMAGE;
  else if (content.Equals("executable"))
    return CPluginSource::EXECUTABLE;
  else if (content.Equals("video"))
    return CPluginSource::VIDEO;
  else
    return CPluginSource::UNKNOWN;
}

} /*namespace ADDON*/


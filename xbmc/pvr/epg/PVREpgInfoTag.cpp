/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "guilib/LocalizeStrings.h"
#include "pvr/channels/PVRChannel.h"
#include "PVREpgInfoTag.h"

using namespace PVR;
using namespace EPG;

void PVR::CPVREpgInfoTag::UpdatePath(void)
{
  if (!m_Epg)
    return;

  CStdString path;
  path.Format("pvr://guide/%04i/%s.epg", m_Epg->Channel() ? m_Epg->Channel()->ChannelID() : m_Epg->EpgID(), m_startTime.GetAsDBDateTime().c_str());
  SetPath(path);
}

const CStdString &PVR::CPVREpgInfoTag::Icon(void) const
{
  if (m_strIconPath.IsEmpty() && m_Epg)
  {
    if (m_Epg->Channel())
      return m_Epg->Channel()->IconPath();
  }

  return m_strIconPath;
}

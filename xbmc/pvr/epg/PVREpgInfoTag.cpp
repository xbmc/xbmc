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

#include "guilib/LocalizeStrings.h"

#include "PVREpg.h"
#include "PVREpgInfoTag.h"
#include "pvr/PVRManager.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "settings/AdvancedSettings.h"

using namespace std;

CPVREpgInfoTag::CPVREpgInfoTag(const EPG_TAG &data)
{
  Reset();
  Update(data);
}

void CPVREpgInfoTag::Reset()
{
  CEpgInfoTag::Reset();

  m_isRecording = false;
  m_Timer       = NULL;
}

const CPVRChannel *CPVREpgInfoTag::ChannelTag(void) const
{
  const CPVREpg *table = (const CPVREpg *) GetTable();
  return table ? table->Channel() : NULL;
}

void CPVREpgInfoTag::SetTimer(const CPVRTimerInfoTag *newTimer)
{
  if (!newTimer)
    m_Timer = NULL;

  m_Timer = newTimer;
}

void CPVREpgInfoTag::UpdatePath(void)
{
  if (!m_Epg)
    return;

  CStdString path;
  path.Format("pvr://guide/channel-%04i/%s.epg", ((CPVREpg *)m_Epg)->Channel()->ChannelNumber(), m_startTime.GetAsDBDateTime().c_str());
  SetPath(path);
}

void CPVREpgInfoTag::Update(const EPG_TAG &tag)
{
  SetStart((time_t) (tag.startTime ? tag.startTime + g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0));
  SetEnd((time_t) (tag.endTime ? tag.endTime + g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0));
  SetTitle(tag.strTitle);
  SetPlotOutline(tag.strPlotOutline);
  SetPlot(tag.strPlot);
  SetGenre(tag.iGenreType, tag.iGenreSubType);
  SetParentalRating(tag.iParentalRating);
  SetUniqueBroadcastID(tag.iUniqueBroadcastId);
  SetNotify(tag.bNotify);
  SetFirstAired((time_t) (tag.firstAired ? tag.firstAired + g_advancedSettings.m_iUserDefinedEPGTimeCorrection : 0));
  SetEpisodeNum(tag.iEpisodeNumber);
  SetEpisodePart(tag.iEpisodePartNumber);
  SetEpisodeName(tag.strEpisodeName);
  SetStarRating(tag.iStarRating);
  SetIcon(tag.strIconPath);
}

const CStdString &CPVREpgInfoTag::Icon(void) const
{
  if (m_strIconPath.IsEmpty() && m_Epg)
  {
    CPVREpg *pvrEpg = (CPVREpg *) m_Epg;
    if (pvrEpg->Channel())
      return pvrEpg->Channel()->IconPath();
  }

  return m_strIconPath;
}

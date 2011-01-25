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
#include "PVRTimers.h"
#include "PVRTimerInfoTag.h"
#include "PVRChannel.h"

using namespace std;

CPVREpgInfoTag::CPVREpgInfoTag(const PVR_PROGINFO &data)
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
  return ((CPVREpg *) GetTable())->Channel();
}

void CPVREpgInfoTag::SetTimer(const CPVRTimerInfoTag *newTimer)
{
  if (!newTimer)
    m_Timer = NULL;

  m_Timer = newTimer;
}

bool CPVREpgInfoTag::HasTimer(void) const
{
  bool bReturn = false;

  if (m_Timer == NULL)
  {
    for (unsigned int i = 0; i < PVRTimers.size(); ++i)
    {
      if (PVRTimers[i].EpgInfoTag() == this)
      {
        bReturn = true;
        break;
      }
    }
  }
  else
  {
    bReturn = true;
  }

  return bReturn;
}

void CPVREpgInfoTag::UpdatePath(void)
{
  if (!m_Epg)
    return;

  CStdString path;
  path.Format("pvr://guide/channel-%04i/%s.epg", ((CPVREpg *)m_Epg)->Channel()->ChannelNumber(), m_startTime.GetAsDBDateTime().c_str());
  SetPath(path);
}

void CPVREpgInfoTag::Update(const PVR_PROGINFO &tag)
{
  SetStart((time_t)tag.starttime);
  SetEnd((time_t)tag.endtime);
  SetTitle(tag.title);
  SetPlotOutline(tag.subtitle);
  SetPlot(tag.description);
  SetGenre(tag.genre_type, tag.genre_sub_type);
  SetParentalRating(tag.parental_rating);
//  SetIcon(((CPVREpg *) m_Epg)->Channel()->IconPath());
}

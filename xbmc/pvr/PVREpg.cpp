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

#include "settings/GUISettings.h"
#include "log.h"
#include "TimeUtils.h"

#include "PVREpg.h"
#include "PVRChannel.h"
#include "PVRManager.h"

#include "epg/EpgContainer.h"
#include "epg/EpgDatabase.h"

CPVREpg::CPVREpg(CPVRChannel *channel) :
  CEpg(channel->ChannelID(), channel->ChannelName(), channel->EPGScraper())
{
  m_Channel  = channel;
}

bool CPVREpg::HasValidEntries(void) const
{
  bool bReturn = CEpg::HasValidEntries();

  if (bReturn)
    bReturn = (m_Channel->ChannelID() > 0); /* valid channel ID */

  return bReturn;
}

void CPVREpg::Cleanup(const CDateTime Time)
{
  EnterCriticalSection(&m_critSection);

  m_bUpdateRunning = true;
  for (unsigned int i = 0; i < size(); i++)
  {
    CPVREpgInfoTag *tag = (CPVREpgInfoTag *) at(i);
    if ( tag && /* valid tag */
        !tag->HasTimer() && /* no timer set */
        (tag->End() + CDateTimeSpan(0, g_PVREpgContainer.m_iLingerTime / 60 + 1, g_PVREpgContainer.m_iLingerTime % 60, 0) < Time)) /* adding one hour for safety */
    {
      DeleteInfoTag(tag);
    }
  }
  m_bUpdateRunning = false;

  LeaveCriticalSection(&m_critSection);
}

bool CPVREpg::UpdateEntry(const PVR_PROGINFO *data, bool bUpdateDatabase /* = false */, bool bEnterCriticalSection /* = true */)
{
  bool bReturn = false;

  if (!data)
    return bReturn;

  if (bEnterCriticalSection)
    EnterCriticalSection(&m_critSection);

  CPVREpgInfoTag tag(*data);
  bReturn = CEpg::UpdateEntry(tag, bUpdateDatabase, false);

  if (bEnterCriticalSection)
    LeaveCriticalSection(&m_critSection);

  return bReturn;
}

bool CPVREpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (ScraperName() == "client")
  {
    if (g_PVRManager.GetClientProps(m_Channel->ClientID())->SupportEPG &&
        g_PVRManager.Clients()->find(m_Channel->ClientID())->second->ReadyToUse())
    {
      bGrabSuccess = g_PVRManager.Clients()->find(m_Channel->ClientID())->second->GetEPGForChannel(*m_Channel, this, start, end) == PVR_ERROR_NO_ERROR;
    }
    else
    {
      CLog::Log(LOGINFO, "%s - client '%s' on client '%i' does not support EPGs",
          __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
    }
  }
  else
  {
    bGrabSuccess = CEpg::UpdateFromScraper(start, end);
  }

  return bGrabSuccess;
}

bool CPVREpg::Load()
{
  /* check if this channel is marked for grabbing */
  if (!m_Channel || !m_Channel->EPGEnabled())
    return false;

  return CEpg::Load();
}

bool CPVREpg::IsRadio(void) const
{
  return m_Channel->IsRadio();
}

bool CPVREpg::Update(const CEpg &epg, bool bUpdateDb /* = false */)
{
  bool bReturn = CEpg::Update(epg, false); // don't update the db yet

  m_Channel = epg.m_Channel;

  if (bUpdateDb)
    bReturn = Persist(false);

  return bReturn;
}

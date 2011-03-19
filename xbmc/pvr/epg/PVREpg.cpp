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

#include "settings/AdvancedSettings.h"
#include "settings/GUISettings.h"
#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

#include "PVREpg.h"
#include "pvr/channels/PVRChannel.h"
#include "pvr/PVRManager.h"

#include "epg/EpgContainer.h"
#include "epg/EpgDatabase.h"

CPVREpg::CPVREpg(CPVRChannel *channel) :
  CEpg(channel->ChannelID(), channel->ChannelName(), channel->EPGScraper())
{
  m_Channel     = channel;
  m_bHasChannel = true;
}

bool CPVREpg::HasValidEntries(void) const
{
  return m_Channel->ChannelID() > 0 && CEpg::HasValidEntries();
}

void CPVREpg::Cleanup(const CDateTime &Time)
{
  CSingleLock lock(m_critSection);

  for (unsigned int i = 0; i < size(); i++)
  {
    CPVREpgInfoTag *tag = (CPVREpgInfoTag *) at(i);
    if ( tag && /* valid tag */
        !tag->HasTimer() && /* no timer set */
        (tag->End() + CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0) < Time))
    {
      DeleteInfoTag(tag);
    }
  }
}

bool CPVREpg::UpdateEntry(const PVR_PROGINFO *data, bool bUpdateDatabase /* = false */)
{
  if (!data)
    return false;

  CPVREpgInfoTag tag(*data);
  return CEpg::UpdateEntry(tag, bUpdateDatabase);
}

bool CPVREpg::UpdateFromScraper(time_t start, time_t end)
{
  bool bGrabSuccess = false;

  if (m_Channel && m_Channel->EPGEnabled() && ScraperName() == "client")
  {
    CLIENTMAP clients;
    if (CPVRManager::Get()->GetClientProperties(m_Channel->ClientID())->SupportEPG &&
        CPVRManager::Get()->GetClients()->Clients(&clients) &&
        clients.find(m_Channel->ClientID())->second->ReadyToUse())
    {
      CLog::Log(LOGINFO, "%s - updating EPG for channel '%s' from client '%i'",
          __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
      bGrabSuccess = clients.find(m_Channel->ClientID())->second->GetEPGForChannel(*m_Channel, this, start, end) == PVR_ERROR_NO_ERROR;
    }
    else
    {
      CLog::Log(LOGINFO, "%s - channel '%s' on client '%i' does not support EPGs",
          __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
    }
  }
  else
  {
    bGrabSuccess = CEpg::UpdateFromScraper(start, end);
  }

  return bGrabSuccess;
}

bool CPVREpg::IsRadio(void) const
{
  return m_Channel->IsRadio();
}

bool CPVREpg::Update(const CEpg &epg, bool bUpdateDb /* = false */)
{
  bool bReturn = CEpg::Update(epg, false); // don't update the db yet

  m_Channel     = epg.m_Channel;
  m_bHasChannel = true;

  if (bUpdateDb)
    bReturn = Persist(false);

  return bReturn;
}

CEpgInfoTag *CPVREpg::CreateTag(void)
{
  CEpgInfoTag *newTag = new CPVREpgInfoTag();
  if (!newTag)
  {
    CLog::Log(LOGERROR, "PVREPG - %s - couldn't create new infotag",
        __FUNCTION__);
  }

  return newTag;
}

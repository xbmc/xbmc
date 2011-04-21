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
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/addons/PVRClients.h"
#include "epg/EpgContainer.h"
#include "epg/EpgDatabase.h"

using namespace PVR;
using namespace EPG;

CPVREpg::CPVREpg(CPVRChannel *channel) :
  CEpg(channel->ChannelID(), channel->ChannelName(), channel->EPGScraper())
{
  SetChannel(channel);
}

bool CPVREpg::HasValidEntries(void) const
{
  CSingleLock lock(m_critSection);

  return m_Channel != NULL && m_Channel->ChannelID() > 0 && CEpg::HasValidEntries();
}

void CPVREpg::Cleanup(const CDateTime &Time)
{
  CSingleLock lock(m_critSection);

  CDateTime firstDate = Time.GetAsUTCDateTime() - CDateTimeSpan(0, g_advancedSettings.m_iEpgLingerTime / 60, g_advancedSettings.m_iEpgLingerTime % 60, 0);

  unsigned int iSize = size();
  for (unsigned int iTagPtr = 0; iTagPtr < iSize; iTagPtr++)
  {
    CPVREpgInfoTag *tag = (CPVREpgInfoTag *) at(iTagPtr);
    if ( tag && /* valid tag */
        !tag->HasTimer() && /* no timer set */
        tag->EndAsLocalTime() < firstDate)
    {
      DeleteInfoTag(tag);
      iTagPtr--;
      iSize--;
    }
  }
}

void CPVREpg::Clear(void)
{
  CSingleLock lock(m_critSection);

  if (m_Channel)
    m_Channel->m_EPG = NULL;

  CEpg::Clear();
}

bool CPVREpg::UpdateEntry(const EPG_TAG *data, bool bUpdateDatabase /* = false */)
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
    if (g_PVRClients->GetClientProperties(m_Channel->ClientID())->bSupportsEPG)
    {
      CLog::Log(LOGINFO, "%s - updating EPG for channel '%s' from client '%i'",
          __FUNCTION__, m_Channel->ChannelName().c_str(), m_Channel->ClientID());
      PVR_ERROR error;
      g_PVRClients->GetEPGForChannel(*m_Channel, this, start, end, &error);
      bGrabSuccess = error == PVR_ERROR_NO_ERROR;
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

  SetChannel((CPVRChannel*) epg.Channel());

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

bool CPVREpg::LoadFromClients(time_t start, time_t end)
{
  bool bReturn(false);
  CPVREpg tmpEpg(m_Channel);
  if (tmpEpg.UpdateFromScraper(start, end))
    bReturn = UpdateEntries(tmpEpg, !g_guiSettings.GetBool("epg.ignoredbforclient"));

  return bReturn;
}

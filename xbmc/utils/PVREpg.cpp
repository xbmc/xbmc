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

#include "PVREpg.h"
#include "PVRChannels.h"
#include "PVREpgInfoTag.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "utils/log.h"
#include "utils/TimeUtils.h"

struct sortEPGbyDate
{
  bool operator()(CPVREpgInfoTag* strItem1, CPVREpgInfoTag* strItem2)
  {
    if (!strItem1 || !strItem2)
      return false;

    return strItem1->Start() < strItem2->Start();
  }
};

CPVREpg::CPVREpg(long ChannelID)
{
  m_channelID       = ChannelID;
  m_Channel         = CPVRChannels::GetByChannelIDFromAll(ChannelID);
  m_bUpdateRunning  = false;
  m_bValid          = ChannelID != -1;
}

CPVREpg::CPVREpg(const CPVRChannel &channel)
{
  m_channelID       = channel.ChannelID();
  m_Channel         = &channel;
  m_bUpdateRunning  = false;
  m_bValid          = m_channelID != -1;
}

bool CPVREpg::IsValid(void) const
{
  if (!m_bValid || m_tags.size() == 0)
    return false;

  if (m_tags[m_tags.size()-1]->m_endTime < CDateTime::GetCurrentDateTime())
    return false;

  return true;
}

CPVREpgInfoTag *CPVREpg::AddInfoTag(CPVREpgInfoTag *Tag)
{
  m_tags.push_back(Tag);
  return Tag;
}

void CPVREpg::DelInfoTag(CPVREpgInfoTag *tag)
{
  if (tag->m_Epg == this)
  {
    for (unsigned int i = 0; i < m_tags.size(); i++)
    {
      CPVREpgInfoTag *entry = m_tags[i];
      if (entry == tag)
      {
        delete entry;
        m_tags.erase(m_tags.begin()+i);
        return;
      }
    }
  }
}

void CPVREpg::Sort(void)
{
  sort(m_tags.begin(), m_tags.end(), sortEPGbyDate());
}

void CPVREpg::Cleanup(void)
{
  Cleanup(CDateTime::GetCurrentDateTime());
}

void CPVREpg::Cleanup(const CDateTime Time)
{
  m_bUpdateRunning = true;
  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    CPVREpgInfoTag *tag = m_tags[i];
    if (!tag->HasTimer() && (tag->End()+CDateTimeSpan(0, g_guiSettings.GetInt("pvrmenu.lingertime") / 60 + 1, g_guiSettings.GetInt("pvrmenu.lingertime") % 60, 0) < Time)) // adding one hour for safety
    {
      DelInfoTag(tag);
    }
    else
      break;
  }
  m_bUpdateRunning = false;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTagNow(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (m_tags.size() == 0)
    return NULL;

  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if ((m_tags[i]->Start() <= now) && (m_tags[i]->End() > now))
      return m_tags[i];
  }
  return NULL;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTagNext(void) const
{
  CDateTime now = CDateTime::GetCurrentDateTime();

  if (m_tags.size() == 0)
    return false;

  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if ((m_tags[i]->Start() <= now) && (m_tags[i]->End() > now))
    {
      CDateTime next = m_tags[i]->End();

      for (unsigned int j = 0; j < m_tags.size(); j++)
      {
        if (m_tags[j]->Start() >= next)
        {
          return m_tags[j];
        }
      }
    }
  }

  return NULL;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTag(long uniqueID, CDateTime StartTime) const
{
  if (uniqueID > 0)
  {
    for (unsigned int i = 0; i < m_tags.size(); i++)
    {
      if (m_tags[i]->UniqueBroadcastID() == uniqueID)
        return m_tags[i];
    }
  }
  else
  {
    for (unsigned int i = 0; i < m_tags.size(); i++)
    {
      if (m_tags[i]->Start() == StartTime)
        return m_tags[i];
    }
  }

  return NULL;
}

const CPVREpgInfoTag *CPVREpg::GetInfoTagAround(CDateTime Time) const
{
  if (m_tags.size() == 0)
    return NULL;

  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if ((m_tags[i]->Start() <= Time) && (m_tags[i]->End() >= Time))
      return m_tags[i];
  }
  return NULL;
}

CDateTime CPVREpg::GetLastEPGDate()
{
  CDateTime last = CDateTime::GetCurrentDateTime();
  for (unsigned int i = 0; i < m_tags.size(); i++)
  {
    if (m_tags[i]->End() >= last)
      last = m_tags[i]->End();
  }
  return last;
}

bool CPVREpg::Add(const PVR_PROGINFO *data, CPVREpg *Epg)
{
  if (Epg && data)
  {
    CPVREpgInfoTag *InfoTag     = NULL;
    CPVREpgInfoTag *newInfoTag  = NULL;
    long uniqueBroadcastID      = data->uid;

    InfoTag = (CPVREpgInfoTag *)Epg->GetInfoTag(uniqueBroadcastID, data->starttime);
    if (!InfoTag)
      InfoTag = newInfoTag = new CPVREpgInfoTag(uniqueBroadcastID);

    if (InfoTag)
    {
      CStdString path;
      path.Format("pvr://guide/channel-%04i/%s.epg", Epg->m_Channel->ChannelNumber(), InfoTag->Start().GetAsDBDateTime().c_str());
      InfoTag->SetPath(path);
      InfoTag->SetStart((time_t)data->starttime);
      InfoTag->SetEnd((time_t)data->endtime);
      InfoTag->SetTitle(data->title);
      InfoTag->SetPlotOutline(data->subtitle);
      InfoTag->SetPlot(data->description);
      InfoTag->SetGenre(data->genre_type, data->genre_sub_type);
      InfoTag->SetParentalRating(data->parental_rating);
      InfoTag->SetIcon(Epg->m_Channel->Icon());
      InfoTag->m_Epg = Epg;

      if (newInfoTag)
        Epg->AddInfoTag(newInfoTag);

      return true;
    }
  }
  return false;
}

bool CPVREpg::AddDB(const PVR_PROGINFO *data, CPVREpg *Epg)
{
  if (Epg && data)
  {
    CTVDatabase *database = g_PVRManager.GetTVDatabase();
    CPVREpgInfoTag InfoTag(data->uid);

    /// NOTE: Database is already opened by the EPG Update function
    CStdString path;
    path.Format("pvr://guide/channel-%04i/%s.epg", Epg->m_Channel->ChannelNumber(), InfoTag.Start().GetAsDBDateTime().c_str());
    InfoTag.SetPath(path);
    InfoTag.SetStart(CDateTime((time_t)data->starttime));
    InfoTag.SetEnd(CDateTime((time_t)data->endtime));
    InfoTag.SetTitle(data->title);
    InfoTag.SetPlotOutline(data->subtitle);
    InfoTag.SetPlot(data->description);
    InfoTag.SetGenre(data->genre_type, data->genre_sub_type);
    InfoTag.SetParentalRating(data->parental_rating);
    InfoTag.SetIcon(Epg->m_Channel->Icon());
    InfoTag.m_Epg = Epg;

    return database->UpdateEPGEntry(InfoTag);
  }
  return false;
}

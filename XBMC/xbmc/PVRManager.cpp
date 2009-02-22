/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#include "stdafx.h"
#include "Directory.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "Application.h"
#include "utils/EPG.h"
#include "TVDatabase.h"
#include "PlayListPlayer.h"
#include "PlayListFactory.h"
#include "utils/TVEPGInfoTag.h"
#include "PlayList.h"
#include "FileSystem/SpecialProtocol.h"
#include "FileSystem/Directory.h"
#include "Util.h"
#include "pvrclients/PVRClientFactory.h"

/*****************************************/

#define XBMC_PVRMANAGER_VERSION "0.2"

using namespace DIRECTORY;

CPVRManager* CPVRManager::m_instance=NULL;
bool CPVRManager::m_hasRecordings = false;
bool CPVRManager::m_isRecording = false;
bool CPVRManager::m_hasTimer = false;
bool CPVRManager::m_hasTimers = false;

/************************************************************/
/** Class handling */

CPVRManager::CPVRManager() {

  //m_CurrentTVChannel      = 1;
  //m_CurrentRadioChannel   = 1;
  //m_CurrentChannelID      = -1;
  //m_HiddenChannels        = 0;

  InitializeCriticalSection(&m_critSection);
}

CPVRManager::~CPVRManager() {

  DeleteCriticalSection(&m_critSection);
  CLog::Log(LOGINFO,"PVR: destroyed");
}

void CPVRManager::Start()
{
  /* First remove any clients */
  if (!m_clients.empty())
    m_clients.clear();

  if (!g_guiSettings.GetBool("pvrmanager.enabled"))
    return;

  CLog::Log(LOGNOTICE, "PVR: pvrmanager starting");

  /* Discover and load chosen plugins */
  if (!LoadClients()) {
    CLog::Log(LOGERROR, "PVR: couldn't load clients");
    return;
  }


  /* Now that clients have been initialized, we check connectivity */
  if (!CheckClientConnections())
    return;

  // check if there are new channels since last connection
  UpdateChannelsList();

  // finally sync info for the infomanager
  //SyncInfo();

  CLog::Log(LOGNOTICE, "PVR: PVRManager started. Clients loaded = %u", m_clients.size());
}

void CPVRManager::Stop()
{
  for (unsigned int i=0; i < m_clients.size(); i++) {
    delete m_clients[i];
  }
  m_clients.clear();
  CLog::Log(LOGNOTICE, "PVR: pvrmanager stopped");
}

/************************************************************/
/** Manager access */

CPVRManager* CPVRManager::GetInstance() {
  if (!m_instance)
    m_instance = new CPVRManager();

  return m_instance;
}

void CPVRManager::ReleaseInstance() {
  m_instance = NULL; /// check is this enough?
}

void CPVRManager::RemoveInstance() {

  if (m_instance) {
    delete m_instance;
    m_instance = NULL;
  }
}

/************************************************************/
/** Thread handling */

void CPVRManager::OnStartup() {
}

void CPVRManager::OnExit() {
}

void CPVRManager::Process() {

  CDateTime lastUpdate = NULL;
  CDateTime lastScan   = NULL;

  //m_database.Open();

  ///* create EPG data structures */
  //if (m_clientProps.SupportEPG) {
  //  bool getClientEPG;

  //  /* Check database for available entries after grid start date */
  //  if (m_database.GetEPGDataEnd(m_currentClientID, -1) <= CDateTime::GetCurrentDateTime()) {
  //    getClientEPG = true;
  //  }
  //  else {
  //    getClientEPG = false;
  //  }

  //  if (getClientEPG) {
  //    for (unsigned int i = 0; i < m_channels_tv.size(); i++) {
  //      m_client->GetEPGForChannel(m_channels_tv[i].m_iClientNum, m_channels_tv[i].m_EPG, lastUpdate, NULL);
  //      for (int j = 0; j < m_channels_tv[i].m_EPG.size(); ++j) {
  //        CTVEPGInfoTag epgentry(NULL);

  //        epgentry.m_strChannel        = m_channels_tv[i].m_strChannel;
  //        epgentry.m_strTitle          = m_channels_tv[i].m_EPG[j].m_strTitle;
  //        epgentry.m_strPlotOutline    = m_channels_tv[i].m_EPG[j].m_strPlotOutline;
  //        epgentry.m_strPlot           = m_channels_tv[i].m_EPG[j].m_strPlot;
  //        epgentry.m_GenreType         = m_channels_tv[i].m_EPG[j].m_GenreType;
  //        epgentry.m_GenreSubType      = m_channels_tv[i].m_EPG[j].m_GenreSubType;
  //        epgentry.m_strGenre          = m_channels_tv[i].m_EPG[j].m_strGenre;
  //        epgentry.m_startTime         = m_channels_tv[i].m_EPG[j].m_startTime;
  //        epgentry.m_endTime           = m_channels_tv[i].m_EPG[j].m_endTime;
  //        epgentry.m_duration          = m_channels_tv[i].m_EPG[j].m_duration;
  //        epgentry.m_channelNum        = m_channels_tv[i].m_iChannelNum;
  //        epgentry.m_idChannel         = m_channels_tv[i].m_iIdChannel;

  //        m_database.AddEPG(m_currentClientID, epgentry);
  //      }
  //    }
  //    for (unsigned int i = 0; i < m_channels_radio.size(); i++) {
  //      m_client->GetEPGForChannel(m_channels_radio[i].m_iClientNum, m_channels_radio[i].m_EPG, lastUpdate, NULL);
  //      for (int j = 0; j < m_channels_radio[i].m_EPG.size(); ++j) {
  //        CTVEPGInfoTag epgentry(NULL);

  //        epgentry.m_strChannel        = m_channels_radio[i].m_strChannel;
  //        epgentry.m_strTitle          = m_channels_radio[i].m_EPG[j].m_strTitle;
  //        epgentry.m_strPlotOutline    = m_channels_radio[i].m_EPG[j].m_strPlotOutline;
  //        epgentry.m_strPlot           = m_channels_radio[i].m_EPG[j].m_strPlot;
  //        epgentry.m_GenreType         = m_channels_radio[i].m_EPG[j].m_GenreType;
  //        epgentry.m_GenreSubType      = m_channels_radio[i].m_EPG[j].m_GenreSubType;
  //        epgentry.m_strGenre          = m_channels_radio[i].m_EPG[j].m_strGenre;
  //        epgentry.m_startTime         = m_channels_radio[i].m_EPG[j].m_startTime;
  //        epgentry.m_endTime           = m_channels_radio[i].m_EPG[j].m_endTime;
  //        epgentry.m_duration          = m_channels_radio[i].m_EPG[j].m_duration;
  //        epgentry.m_channelNum        = m_channels_radio[i].m_iChannelNum;
  //        epgentry.m_idChannel         = m_channels_radio[i].m_iIdChannel;

  //        m_database.AddEPG(m_currentClientID, epgentry);
  //      }
  //    }
  //    m_database.Compress(true);
  //  }
  //  else {
  //    CDateTime start = CDateTime::GetCurrentDateTime()-CDateTimeSpan(0, g_guiSettings.GetInt("pvrmenu.lingertime") / 60, g_guiSettings.GetInt("pvrmenu.lingertime") % 60, 0);
  //    CDateTime end   = CDateTime::GetCurrentDateTime()+CDateTimeSpan(g_guiSettings.GetInt("pvrmenu.daystodisplay"), 0, 0, 0);

  //    for (unsigned int i = 0; i < m_channels_tv.size(); i++) {
  //      m_database.GetEPGForChannel(m_currentClientID, m_channels_tv[i].m_iIdChannel, m_channels_tv[i].m_EPG, start, end);
  //    }
  //    for (unsigned int i = 0; i < m_channels_radio.size(); i++) {
  //      m_database.GetEPGForChannel(m_currentClientID, m_channels_radio[i].m_iIdChannel, m_channels_radio[i].m_EPG, start, end);
  //    }
  //  }
  //}

  //m_database.Close();

  while (!m_bStop) {
    /*VECCHANNELS     m_channels_tmp;
    VECTVTIMERS     m_timers_tmp;
    VECRECORDINGS   m_recordings_tmp;*/
    /*-------------------------------------------------*/
    /* Look for timers change */
    //if (m_clientProps.SupportTimers) {
    //  EnterCriticalSection(&m_critSection);

    //  if (m_client->GetNumTimers() > 0) {
    //    m_client->GetAllTimers(&m_timers_tmp);
    //  }
    //  if (m_timers.size() > 0) {
    //    if (m_timers.size() != m_timers_tmp.size()) {
    //      UpdateTimersList();
    //    }
    //    else {
    //      for (unsigned int i = 0; i < m_timers_tmp.size(); ++i) {
    //        if (m_timers_tmp[i] != m_timers[i]) {
    //          UpdateTimersList();
    //          break;
    //        }
    //      }
    //    }
    //  }
    //  else if (m_timers_tmp.size() > 0) {
    //    UpdateTimersList();
    //  }
    //  LeaveCriticalSection(&m_critSection);
    //}
    ///* Wait 10 seconds until start next change check */
    Sleep(10000);

    /*-------------------------------------------------*/
    /* Look for recordings change */
    //if (m_clientProps.SupportRecordings) {
    //  EnterCriticalSection(&m_critSection);

    //  if (m_client->GetNumRecordings() > 0) {
    //    m_client->GetAllRecordings(&m_recordings_tmp);
    //  }
    //  if (m_recordings.size() > 0) {
    //    if (m_recordings.size() != m_recordings_tmp.size()) {
    //      UpdateRecordingList();
    //    }
    //    else {
    //      for (unsigned int i = 0; i < m_recordings_tmp.size(); ++i) {
    //        if (m_recordings_tmp[i] != m_recordings[i]) {
    //          UpdateRecordingList();
    //          break;
    //        }
    //      }
    //    }
    //  }
    //  else if (m_recordings_tmp.size() > 0) {
    //    UpdateRecordingList();
    //  }
    //  LeaveCriticalSection(&m_critSection);
    //}
    /* Wait 5 seconds until start next change check */
    Sleep(10000);

    /*-------------------------------------------------*/
    /* Look for TV channels change */
    //{
    //  EnterCriticalSection(&m_critSection);
    //  m_database.Open();

    //  if (m_client->GetNumChannels() > 0) {
    //    m_client->GetAllChannels(&m_channels_tmp, false);
    //  }
    //  if (m_channels_tv.size() > 0) {
    //    if (m_channels_tmp.size() > m_channels_tv.size()) {
    //      for (unsigned int i = 0; i < m_channels_tmp.size(); i++) {
    //        if (!m_database.HasChannel(m_currentClientID, m_channels_tmp[i])) {
    //          m_database.UpdateChannel(m_currentClientID, m_channels_tmp[i]);
    //        }
    //      }
    //      m_database.GetChannelList(m_currentClientID, &m_channels_tv, false);
    //      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);

    //    }
    //    else if (m_channels_tmp.size() < m_channels_tv.size()) {
    //      for (unsigned int i = 0; i < m_channels_tmp.size(); i++) {
    //        /// TODO: Remove wrong/old channels


    //      }
    //      m_database.GetChannelList(m_currentClientID, &m_channels_tv, false);
    //      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);

    //    }
    //    else {
    //      int updated = 0;
    //      for (unsigned int i = 0; i < m_channels_tmp.size(); ++i) {
    //        if (m_channels_tmp[i].m_iClientNum != m_channels_tv[i].m_iClientNum &&
    //          m_channels_tmp[i].m_strChannel != m_channels_tv[i].m_strChannel) {

    //            m_channels_tv[i].m_iClientNum = m_channels_tmp[i].m_iClientNum;
    //            m_channels_tv[i].m_strChannel = m_channels_tmp[i].m_strChannel;
    //            m_database.UpdateChannel(m_currentClientID, m_channels_tv[i]);
    //            updated++;
    //        }
    //      }
    //      if (updated > 0) {
    //        m_database.GetChannelList(m_currentClientID, &m_channels_tv, false);
    //      }
    //    }
    //  }
    //  else if (m_channels_tmp.size() > 0) {
    //    /* Fill Channels to Database */
    //    for (unsigned int i = 0; i < m_channels_tmp.size(); i++) {
    //      m_database.AddChannel(m_currentClientID, m_channels_tmp[i]);
    //    }
    //    m_database.GetChannelList(m_currentClientID, &m_channels_tv, false);
    //  }
    //  m_database.Close();
    //  LeaveCriticalSection(&m_critSection);
    //}
    ///* Wait 10 seconds until start next change check */
    //Sleep(10000);

    ///*-------------------------------------------------*/
    ///* Look for Radio channels change */
    //if (m_clientProps.SupportRadio) {
    //  EnterCriticalSection(&m_critSection);
    //  m_database.Open();

    //  if (m_client->GetNumChannels() > 0) {
    //    m_client->GetAllChannels(&m_channels_tmp, true);
    //  }
    //  if (m_channels_radio.size() > 0) {
    //    if (m_channels_tmp.size() > m_channels_radio.size()) {
    //      for (unsigned int i = 0; i < m_channels_tmp.size(); i++) {
    //        if (!m_database.HasChannel(m_currentClientID, m_channels_tmp[i])) {
    //          m_database.UpdateChannel(m_currentClientID, m_channels_tmp[i]);
    //        }
    //      }
    //      m_database.GetChannelList(m_currentClientID, &m_channels_radio, true);
    //      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);

    //    }
    //    else if (m_channels_tmp.size() < m_channels_radio.size()) {
    //      for (unsigned int i = 0; i < m_channels_tmp.size(); i++) {
    //        /// TODO: Remove wrong/old channels


    //      }
    //      m_database.GetChannelList(m_currentClientID, &m_channels_radio, true);
    //      m_HiddenChannels = m_database.GetNumHiddenChannels(m_currentClientID);

    //    }
    //    else {
    //      int updated = 0;
    //      for (unsigned int i = 0; i < m_channels_tmp.size(); ++i) {
    //        if (m_channels_tmp[i].m_iClientNum != m_channels_radio[i].m_iClientNum &&
    //          m_channels_tmp[i].m_strChannel != m_channels_radio[i].m_strChannel) {

    //            m_channels_radio[i].m_iClientNum = m_channels_tmp[i].m_iClientNum;
    //            m_channels_radio[i].m_strChannel = m_channels_tmp[i].m_strChannel;
    //            m_database.UpdateChannel(m_currentClientID, m_channels_radio[i]);
    //            updated++;
    //        }
    //      }
    //      if (updated > 0) {
    //        m_database.GetChannelList(m_currentClientID, &m_channels_radio, true);
    //      }
    //    }
    //  }
    //  else if (m_channels_tmp.size() > 0) {
    //    /* Fill Channels to Database */
    //    for (unsigned int i = 0; i < m_channels_tmp.size(); i++) {
    //      m_database.AddChannel(m_currentClientID, m_channels_tmp[i]);
    //    }
    //    m_database.GetChannelList(m_currentClientID, &m_channels_radio, true);
    //  }
    //  m_database.Close();
    //  LeaveCriticalSection(&m_critSection);
    //}
    ///* Wait 10 seconds until start next change check */
    Sleep(10000);

    /*-------------------------------------------------*/
    /* Look for epg change */
    //if (m_clientProps.SupportEPG) {
      /*-------------------------------------------------*/
      /* tv
      /*/
      /*     if (lastUpdate+CDateTimeSpan(0, g_guiSettings.GetInt("pvrepg.epgupdate") / 60, g_guiSettings.GetInt("pvrepg.epgupdate") % 60, 0) < CDateTime::GetCurrentDateTime()) {
      lastUpdate = CDateTime::GetCurrentDateTime();
      m_database.Open();

      if (m_channels_tv.size() > 0) {
      bool changed = false;
      CDateTime now = CDateTime::GetCurrentDateTime();

      for (unsigned int i = 0; i < m_channels_tv.size(); ++i) {
      EPG_DATA epglist;

      CDateTime lastEntry = m_database.GetEPGDataEnd(m_currentClientID, m_channels_tv[i].m_iIdChannel);

      if (lastEntry <= now) {
      m_client->GetEPGForChannel(m_channels_tv[i].m_iClientNum, epglist);
      for (int j = 0; j < epglist.size(); ++j) {
      CTVEPGInfoTag epgentry(NULL);

      epgentry.m_strChannel        = m_channels_tv[i].m_strChannel;
      epgentry.m_strTitle          = epglist[j].m_strTitle;
      epgentry.m_strPlotOutline    = epglist[j].m_strPlotOutline;
      epgentry.m_strPlot           = epglist[j].m_strPlot;
      epgentry.m_GenreType         = epglist[j].m_GenreType;
      epgentry.m_GenreSubType      = epglist[j].m_GenreSubType;
      epgentry.m_strGenre          = epglist[j].m_strGenre;
      epgentry.m_startTime         = epglist[j].m_startTime;
      epgentry.m_endTime           = epglist[j].m_endTime;
      epgentry.m_duration          = epglist[j].m_duration;
      epgentry.m_channelNum        = m_channels_tv[i].m_iChannelNum;
      epgentry.m_idChannel         = m_channels_tv[i].m_iIdChannel;
      epgentry.m_isRadio           = m_channels_tv[i].m_radio;
      epgentry.m_IconPath          = m_channels_tv[i].m_IconPath;

      EnterCriticalSection(&m_critSection);
      m_database.AddEPG(m_currentClientID, epgentry);
      LeaveCriticalSection(&m_critSection);
      }
      if (epglist.size() > 0) changed = true;
      }
      else {
      m_client->GetEPGForChannel(m_channels_tv[i].m_iClientNum, epglist, lastEntry);
      for (int j = 0; j < epglist.size(); ++j) {
      CTVEPGInfoTag epgentry(NULL);

      epgentry.m_strChannel        = m_channels_tv[i].m_strChannel;
      epgentry.m_strTitle          = epglist[j].m_strTitle;
      epgentry.m_strPlotOutline    = epglist[j].m_strPlotOutline;
      epgentry.m_strPlot           = epglist[j].m_strPlot;
      epgentry.m_GenreType         = epglist[j].m_GenreType;
      epgentry.m_GenreSubType      = epglist[j].m_GenreSubType;
      epgentry.m_strGenre          = epglist[j].m_strGenre;
      epgentry.m_startTime         = epglist[j].m_startTime;
      epgentry.m_endTime           = epglist[j].m_endTime;
      epgentry.m_duration          = epglist[j].m_duration;
      epgentry.m_channelNum        = m_channels_tv[i].m_iChannelNum;
      epgentry.m_idChannel         = m_channels_tv[i].m_iIdChannel;
      epgentry.m_isRadio           = m_channels_tv[i].m_radio;
      epgentry.m_IconPath          = m_channels_tv[i].m_IconPath;

      EnterCriticalSection(&m_critSection);
      m_database.AddEPG(m_currentClientID, epgentry);
      LeaveCriticalSection(&m_critSection);
      }
      if (epglist.size() > 0) changed = true;
      }
      if (changed) {
      CDateTime end = CDateTime::GetCurrentDateTime()+CDateTimeSpan(g_guiSettings.GetInt("pvrmenu.daystodisplay"), 0, 0, 0);
      for (unsigned int i = 0; i < m_channels_tv.size(); i++) {
      CTVEPGInfoTag epglast(NULL);

      EnterCriticalSection(&m_critSection);
      m_channels_tv[i].GetEPGLastEntry(&epglast);
      m_database.GetEPGForChannel(m_currentClientID, m_channels_tv[i].m_iIdChannel, m_channels_tv[i].m_EPG, epglast.m_endTime, end);
      LeaveCriticalSection(&m_critSection);
      }
      EnterCriticalSection(&m_critSection);
      m_database.Compress(true);
      LeaveCriticalSection(&m_critSection);
      }
      }
      }
      m_database.Close();
      }*/
      /* Wait 10 seconds until start next change check */
      Sleep(10000);

      /*-------------------------------------------------*/
      /* radio
      /*/
    //  if (m_clientProps.SupportRadio) {
    //    if (lastUpdate+CDateTimeSpan(0, g_guiSettings.GetInt("pvrepg.epgupdate") / 60, g_guiSettings.GetInt("pvrepg.epgupdate") % 60+5, 0) < CDateTime::GetCurrentDateTime()) {
    //      lastUpdate = CDateTime::GetCurrentDateTime();

    //      m_database.Open();

    //      if (m_channels_radio.size() > 0) {
    //        bool changed = false;
    //        CDateTime now = CDateTime::GetCurrentDateTime();

    //        for (unsigned int i = 0; i < m_channels_radio.size(); ++i) {
    //          EPG_DATA epglist;

    //          CDateTime lastEntry = m_database.GetEPGDataEnd(m_currentClientID, m_channels_radio[i].m_iIdChannel);

    //          if (lastEntry <= now) {
    //            m_client->GetEPGForChannel(m_channels_radio[i].m_iClientNum, epglist);
    //            for (int j = 0; j < epglist.size(); ++j) {
    //              CTVEPGInfoTag epgentry(NULL);

    //              epgentry.m_strChannel        = m_channels_radio[i].m_strChannel;
    //              epgentry.m_strTitle          = epglist[j].m_strTitle;
    //              epgentry.m_strPlotOutline    = epglist[j].m_strPlotOutline;
    //              epgentry.m_strPlot           = epglist[j].m_strPlot;
    //              epgentry.m_GenreType         = epglist[j].m_GenreType;
    //              epgentry.m_GenreSubType      = epglist[j].m_GenreSubType;
    //              epgentry.m_strGenre          = epglist[j].m_strGenre;
    //              epgentry.m_startTime         = epglist[j].m_startTime;
    //              epgentry.m_endTime           = epglist[j].m_endTime;
    //              epgentry.m_duration          = epglist[j].m_duration;
    //              epgentry.m_channelNum        = m_channels_radio[i].m_iChannelNum;
    //              epgentry.m_idChannel         = m_channels_radio[i].m_iIdChannel;
    //              epgentry.m_isRadio           = m_channels_radio[i].m_radio;
    //              epgentry.m_IconPath          = m_channels_radio[i].m_IconPath;

    //              EnterCriticalSection(&m_critSection);
    //              m_database.AddEPG(m_currentClientID, epgentry);
    //              LeaveCriticalSection(&m_critSection);
    //            }
    //            if (epglist.size() > 0) changed = true;
    //          }
    //          else {
    //            m_client->GetEPGForChannel(m_channels_radio[i].m_iClientNum, epglist, lastEntry);
    //            for (int j = 0; j < epglist.size(); ++j) {
    //              CTVEPGInfoTag epgentry(NULL);

    //              epgentry.m_strChannel        = m_channels_radio[i].m_strChannel;
    //              epgentry.m_strTitle          = epglist[j].m_strTitle;
    //              epgentry.m_strPlotOutline    = epglist[j].m_strPlotOutline;
    //              epgentry.m_strPlot           = epglist[j].m_strPlot;
    //              epgentry.m_GenreType         = epglist[j].m_GenreType;
    //              epgentry.m_GenreSubType      = epglist[j].m_GenreSubType;
    //              epgentry.m_strGenre          = epglist[j].m_strGenre;
    //              epgentry.m_startTime         = epglist[j].m_startTime;
    //              epgentry.m_endTime           = epglist[j].m_endTime;
    //              epgentry.m_duration          = epglist[j].m_duration;
    //              epgentry.m_channelNum        = m_channels_radio[i].m_iChannelNum;
    //              epgentry.m_idChannel         = m_channels_radio[i].m_iIdChannel;
    //              epgentry.m_isRadio           = m_channels_radio[i].m_radio;
    //              epgentry.m_IconPath          = m_channels_radio[i].m_IconPath;

    //              EnterCriticalSection(&m_critSection);
    //              m_database.AddEPG(m_currentClientID, epgentry);
    //              LeaveCriticalSection(&m_critSection);
    //            }
    //            if (epglist.size() > 0) changed = true;
    //          }
    //          if (changed) {
    //            CDateTime end = CDateTime::GetCurrentDateTime()+CDateTimeSpan(g_guiSettings.GetInt("pvrmenu.daystodisplay"), 0, 0, 0);
    //            for (unsigned int i = 0; i < m_channels_radio.size(); i++) {
    //              CTVEPGInfoTag epglast(NULL);
    //              EnterCriticalSection(&m_critSection);
    //              m_channels_radio[i].GetEPGLastEntry(&epglast);
    //              m_database.GetEPGForChannel(m_currentClientID, m_channels_radio[i].m_iIdChannel, m_channels_radio[i].m_EPG, epglast.m_endTime, end);
    //              LeaveCriticalSection(&m_critSection);
    //            }
    //            EnterCriticalSection(&m_critSection);
    //            m_database.Compress(true);
    //            LeaveCriticalSection(&m_critSection);
    //          }
    //        }
    //      }
    //      m_database.Close();
    //    }
    //  }
    //  /* Update all entries and delete to old entries inside database */
    //  if (lastScan+CDateTimeSpan(g_guiSettings.GetInt("pvrepg.epgscan") / 24, g_guiSettings.GetInt("pvrepg.epgscan") % 24, 0, 0) < CDateTime::GetCurrentDateTime()) {
    //    lastScan = CDateTime::GetCurrentDateTime();
    //    m_database.Open();

    //    if (m_channels_tv.size() > 0) {
    //      bool changed = false;
    //      CDateTime span = CDateTime::GetCurrentDateTime()-CDateTimeSpan(g_guiSettings.GetInt("pvrepg.daystosave"),0,0,0);

    //      for (unsigned int i = 0; i < m_channels_tv.size(); ++i) {
    //        CDateTime firstEntry = m_database.GetEPGDataStart(m_currentClientID, m_channels_tv[i].m_iIdChannel);
    //        if (firstEntry < span) {
    //          m_database.RemoveEPGEntries(m_currentClientID, m_channels_tv[i].m_iIdChannel, firstEntry, span);
    //          changed = true;
    //        }
    //      }
    //    }
    //    if (m_clientProps.SupportRadio && m_channels_radio.size() > 0) {
    //      bool changed = false;
    //      CDateTime span = CDateTime::GetCurrentDateTime()-CDateTimeSpan(g_guiSettings.GetInt("pvrepg.daystosave"),0,0,0);

    //      for (unsigned int i = 0; i < m_channels_radio.size(); ++i) {
    //        CDateTime firstEntry = m_database.GetEPGDataStart(m_currentClientID, m_channels_radio[i].m_iIdChannel);
    //        if (firstEntry < span) {
    //          m_database.RemoveEPGEntries(m_currentClientID, m_channels_radio[i].m_iIdChannel, firstEntry, span);
    //          changed = true;
    //        }
    //      }
    //    }
    //    m_database.Close();
    //  }
    //}
    /* Wait 30 seconds until start next change check */
    Sleep(30000);
  }
}

/************************************************************/
/** Client handling */

bool CPVRManager::LoadClients()
{
  ScanPluginDirs();

  // retrieve existing client settings from db
  m_database.Open();
  /*m_database.GetClientId(*/

  if (m_plugins.empty())
    return false;

  // load the plugins
  CPVRClientFactory factory;
  for (unsigned i =0; i<m_plugins.size(); i++)
  {
    IPVRClient *client;
    client = factory.LoadPVRClient(m_plugins[i], i, this);
    if (client)
      m_clients.insert(std::make_pair(client->GetID(), client));
  }

  // Request each client's basic properties
  GetClientProperties();

  return !m_clients.empty();
}

void CPVRManager::ScanPluginDirs()
{
  // first clear the known plugins
  if (!m_plugins.empty())
    m_plugins.clear();

  CFileItemList items;
  CDirectory::GetDirectory("special://xbmc/pvrclients/", items);
  if (!CSpecialProtocol::XBMCIsHome())
    CDirectory::GetDirectory("special://home/pvrclients/", items);

  CStdString strExtension;
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      const char *clientPath = (const char*)pItem->m_strPath;

      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".dll")
      {
#ifdef _LINUX
        void *handle = dlopen( _P(clientPath).c_str(), RTLD_LAZY );
        if (!handle)
          continue;
        dlclose(handle);
#endif
        CStdString strLabel = pItem->GetLabel();
        m_plugins.push_back(strLabel); 
      }
    }
  }

  CLog::Log(LOGINFO, "PVR: found %u plugin(s)", m_plugins.size());
}

bool CPVRManager::CheckClientConnections()
{
  std::map< long, IPVRClient* >::iterator clientItr(m_clients.begin());
  while (clientItr != m_clients.end())
  {
    // signal client to connect to backend
    (*clientItr).second->Connect();

    // check client has connected
    if (!(*clientItr).second->IsUp())
      clientItr = m_clients.erase(clientItr);
    else
      ++clientItr;
  }

  if (m_clients.empty())
  {
    CLog::Log(LOGERROR, "PVR: no clients could connect");
    return false;
  }

  return true;
}

//CURL CPVRManager::GetConnString(long clientID)
//{
//  CURL connString;
//
//  // set client defaults
//  connString.SetHostName(
//  connString.SetUserName(m_clientProps[clientID].DefaultUser);
//  connString.SetPassword(m_clientProps[clientID].DefaultPassword);
//  connString.SetPort(m_clientProps[clientID].DefaultPort);
//
//  CStdString host, user, pass, token;
//  int port;
//  token.Format("pvrmanaer.client%u", clientID);
//
//  host = g_guiSettings.GetString(token + ".serverip");
//  user = g_guiSettings.GetString(token + ".username");
//  pass = g_guiSettings.GetString(token + ".password");
//  port = g_guiSettings.GetInt(token + ".serverport");
//
//  if (!host.IsEmpty())
//  {
//    connString.SetHostName(host);
//  }
//  if (!user.IsEmpty())
//  {
//    connString.SetUserName(user);
//  }
//  if (!pass.IsEmpty())
//  {
//    connString.SetPassword(pass);
//  }
//  if (port > 0 && port < 65536)
//  {
//    connString.SetPort(port);
//  }
//
//  return connString;
//}

void CPVRManager::GetClientProperties()
{
  m_clientProps.clear();
  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    GetClientProperties((*itr).first);
    itr++;
  }
}

void CPVRManager::GetClientProperties(long clientID)
{
  PVR_SERVERPROPS props;
  m_clients[clientID]->GetProperties(&props);
  m_clientProps.insert(std::make_pair(clientID, props));
}

bool CPVRManager::IsConnected()
{
  if (m_clients.empty())
    return false;

  return true;
}

const char* CPVRManager::TranslateInfo(DWORD dwInfo)
{
  if (dwInfo == PVR_NOW_RECORDING_CHANNEL) return m_nowRecordingClient;
  else if (dwInfo == PVR_NOW_RECORDING_TITLE) return m_nowRecordingTitle;
  else if (dwInfo == PVR_NOW_RECORDING_DATETIME) return m_nowRecordingDateTime;
  else if (dwInfo == PVR_NEXT_RECORDING_CHANNEL) return m_nextRecordingClient;
  else if (dwInfo == PVR_NEXT_RECORDING_TITLE) return m_nextRecordingTitle;
  else if (dwInfo == PVR_NEXT_RECORDING_DATETIME) return m_nextRecordingDateTime;
  return "";
}

void CPVRManager::OnClientMessage(const long clientID, const PVR_EVENT clientEvent, const char* msg)
{
  /* here the manager reacts to messages sent from any of the clients via the IPVRClientCallback */
  switch (clientEvent) {
    case PVR_EVENT_UNKNOWN:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%u unknown event : %s", __FUNCTION__, clientID, msg);
      break;

    case PVR_EVENT_TIMERS_CHANGE:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%u timers changed", __FUNCTION__, clientID);
      /*GetTimers();*/
      /*GetConflicting(clientID);*/
      /*SyncInfo();*/
      break;

    case PVR_EVENT_RECORDINGS_CHANGE:
      CLog::Log(LOGDEBUG, "%s - PVR: client_%u recording list changed", __FUNCTION__, clientID);
      /*GetTimers();
      GetRecordings();
      SyncInfo();*/
      break;
  }
}

void CPVRManager::UpdateChannelsList()
{
  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    UpdateChannelsList((*itr).first);
    itr++;
  }
}

void CPVRManager::UpdateChannelsList(long clientID)
{
  VECCHANNELS channels;
  if(m_clients[clientID]->GetChannelList(channels) == PVR_ERROR_NO_ERROR)
  {
    // first check there are no stored channels for this client
    std::map< long, VECCHANNELS* >::iterator itr = m_channels.begin();
    while (itr != m_channels.end())
    {
      if (clientID == (*itr).first)
        itr = m_channels.erase(itr);
      else
        itr++;
    }

    // store the timers for this client
    m_channels.insert(std::make_pair(clientID, &channels));
  }
  else
  {
    // couldn't get channel list
    CLog::Log(LOG_ERROR, "PVR: client: %u Error recieving channel list", clientID);
  }
}

void CPVRManager::UpdateChannelData()
{
  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    UpdateChannelData((*itr).first);
    itr++;
  }
}

void CPVRManager::UpdateChannelData(long clientID)
{
//   m_database.Open();
// 
//   CDateTime dataEnd, now;
//   now = CDateTime::GetCurrentDateTime();
//   dataEnd = m_database.GetDataEnd(clientID);
// 
//   CDateTimeSpan  minimum;
//   minimum.SetDateTimeSpan(g_guiSettings.GetInt("pvrmanager.daystodisplay"), 0, 0, 0);
// 
//   if (dataEnd < now + minimum)
//   {
//     EPGData channels;
//     m_database.GetChannelList(clientID, channels);
//     EPGData::iterator itr = channels.begin();
//     while (itr != channels.end())
//     {
//       CTVChannel* channel = *itr;
//       CFileItemList* data = new CFileItemList(channel->Name());
//       m_clients[clientID]->GetEPGForChannel(channel->GetBouquetID(), channel->Number());
//       itr++;
//     }
// 
//   }
// 
//   m_database.Close();
}

//PVRSCHEDULES CPVRManager::GetScheduled()
//{
//  return m_scheduledRecordings;
//}
//
//PVRSCHEDULES CPVRManager::GetConflicting()
//{
//  return m_conflictingSchedules;
//}

 /** Protected ***************************************************************/

void CPVRManager::SyncInfo()
{
  m_numUpcomingSchedules > 0 ? m_hasTimer = true : m_hasTimer = false;
  m_numRecordings > 0 ? m_hasRecordings = true : m_hasRecordings = false;
  m_numTimers > 0 ? m_hasTimers = true : m_hasTimers = false;
  m_isRecording = false;

  //if (m_hasTimer)
  //{
  //  CDateTime nextRec;
  //  for (PVRSCHEDULES::iterator itr = m_scheduledRecordings.begin(); itr != m_scheduledRecordings.end(); itr++)
  //  {
  //    if(!(*itr).second.empty())
  //    { /* this client has recordings scheduled */
  //      CTVTimerInfoTag *item = (*itr).second[0]; // first item will be next scheduled recording
  //      if (nextRec > item->GetEPGInfoTag()->m_startTime || !nextRec.IsValid())
  //      {
  //        nextRec = item->GetEPGInfoTag()->m_startTime;
  //        m_nextRecordingTitle = item->GetEPGInfoTag()->m_strChannel + " - " + item->GetEPGInfoTag()->m_strTitle;
  //        m_nextRecordingClient = m_clientProps[(*itr).first].Name;
  //        m_nextRecordingDateTime = nextRec.GetAsLocalizedDateTime(false, false);
  //        if (item->GetEPGInfoTag()->m_recStatus == rsRecording)
  //          m_isRecording = true;
  //        else
  //          m_isRecording = false;
  //      }
  //    } 
  //  }
  //}

  if (m_isRecording)
  {
    m_nowRecordingTitle = m_nextRecordingTitle;
    m_nowRecordingClient = m_nextRecordingClient;
    m_nowRecordingDateTime = m_nextRecordingDateTime;
  }
  else
  {
    m_nowRecordingTitle.clear();
    m_nowRecordingClient.clear();
    m_nowRecordingDateTime.clear();
  }
}

CStdString CPVRManager::PrintStatus(RecStatus status)
{
  CStdString string;
  switch (status) {
    case rsDeleted:
    case rsStopped:
    case rsRecorded:
    case rsRecording:
    case rsWillRecord:
    case rsUnknown:
    case rsDontRecord:
    case rsPrevRecording:
    case rsCurrentRecording:
    case rsEarlierRecording:
    case rsTooManyRecordings:
    case rsCancelled:
    case rsConflict:
    case rsLaterShowing:
    case rsRepeat:
    case rsLowDiskspace:
    case rsTunerBusy:
      string = g_localizeStrings.Get(17100 + status);
      break;
    default:
      string = g_localizeStrings.Get(17100); // 17100 is "unknown"
  };
  
  return string;
}

void CPVRManager::GetTimers()
{
  m_numTimers = 0;
  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    m_numTimers += GetTimers((*itr).first);
    itr++;
  }
}

void CPVRManager::GetRecordings()
{
  m_numRecordings = 0;

  std::map< long, IPVRClient* >::iterator itr = m_clients.begin();
  while (itr != m_clients.end())
  {
    m_numRecordings += GetRecordings((*itr).first);
    itr++;
  }
}

int CPVRManager::GetTimers(long clientID)
{
  VECTVTIMERS timers;

  //if(m_clients[clientID]->GetTimers(&timers))
  //{
  //  timers->Sort(SORT_METHOD_DATE, SORT_ORDER_ASC);

  //  // first check there are no stored timers for this client
  //  std::map< long, VECTVTIMERS >::iterator itr = m_timers.begin();
  //  while (itr != m_timers.end())
  //  {
  //    if (clientID == (*itr).first)
  //      itr = m_timers.erase(itr);
  //    else
  //      itr++;
  //  }

  //  // store the timers for this client
  //  m_timers.push_back(std::make_pair(clientID, timers));
  //}
  //return timers->Size();
  return 0;
}

int CPVRManager::GetRecordings(long clientID)
{
  /*CFileItemList* recordings = new CFileItemList();
  m_clients[clientID]->GetRecordings(recordings);
  return recordings->Size();*/
  return 0;
}

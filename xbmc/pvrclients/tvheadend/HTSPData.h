#pragma once

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

#include "client.h"
#include "../../../lib/platform/threads/threads.h"
#include "HTSPConnection.h"

class CHTSPData : public PLATFORM::CThread
{
public:
  CHTSPData();
  ~CHTSPData();

  bool Open();
  void Close();
  bool CheckConnection(void);
  bool IsConnected(void) const { return m_session->IsConnected(); }

  /*!
   * @brief Send a message to the backend and read the result.
   * @param message The message to send.
   * @return The returned message or NULL if an error occured or nothing was received.
   */
  htsmsg_t *   ReadResult(htsmsg_t *message);
  int          GetProtocol(void) const   { return m_session->GetProtocol(); }
  const char * GetServerName(void) const { return m_session->GetServerName(); }
  const char * GetVersion(void) const    { return m_session->GetVersion(); }
  bool         GetDriveSpace(long long *total, long long *used);
  bool         GetBackendTime(time_t *utcTime, int *gmtOffset);
  unsigned int GetNumChannels(void);
  PVR_ERROR    GetChannels(PVR_HANDLE handle, bool bRadio);
  PVR_ERROR    GetEpg(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart, time_t iEnd);
  unsigned int GetNumRecordings();
  PVR_ERROR    GetRecordings(PVR_HANDLE handle);
  PVR_ERROR    DeleteRecording(const PVR_RECORDING &recinfo);
  PVR_ERROR    AddTimer(const PVR_TIMER &timerinfo);
  PVR_ERROR    UpdateTimer(const PVR_TIMER &timerinfo);
  PVR_ERROR    RenameRecording(const PVR_RECORDING &recinfo, const char* newname);
  unsigned int GetNumTimers();
  PVR_ERROR    GetTimers(PVR_HANDLE handle);
  PVR_ERROR    DeleteTimer(const PVR_TIMER &timerinfo, bool force);
  unsigned int GetNumChannelGroups(void);
  PVR_ERROR    GetChannelGroups(PVR_HANDLE handle);
  PVR_ERROR    GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group);

protected:
  virtual void *Process(void);

private:
  struct SMessage
  {
    PLATFORM::CCondition* event;
    PLATFORM::CMutex    * mutex;
    htsmsg_t*             msg;
  };
  typedef std::map<int, SMessage> SMessages;

  SChannels GetChannels();
  SChannels GetChannels(int tag);
  SChannels GetChannels(STag &tag);
  STags GetTags();
  bool GetEvent(SEvent& event, uint32_t id);
  bool SendEnableAsync();
  SRecordings GetDVREntries(bool recorded, bool scheduled);

  void ParseChannelRemove(htsmsg_t* msg);
  void ParseChannelUpdate(htsmsg_t* msg);
  void ParseDVREntryDelete(htsmsg_t* msg);
  void ParseDVREntryUpdate(htsmsg_t* msg);
  static bool ParseEvent(htsmsg_t* msg, uint32_t id, SEvent &event);
  void ParseTagRemove(htsmsg_t* msg);
  void ParseTagUpdate(htsmsg_t* msg);

  CHTSPConnection *    m_session;
  PLATFORM::CCondition m_started;
  PLATFORM::CMutex     m_mutex;
  SChannels            m_channels;
  STags                m_tags;
  SEvents              m_events;
  SMessages            m_queue;
  SRecordings          m_recordings;
  int                  m_iReconnectRetries;
  bool                 m_bDisconnectWarningDisplayed;
};


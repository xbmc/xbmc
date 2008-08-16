#pragma once

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

#include "FileItem.h"

typedef enum {
  PVRCLIENT_EVENT_UNKNOWN = 20,
  PVRCLIENT_EVENT_CLOSE,
  PVRCLIENT_EVENT_RECORDING_LIST_CHANGE,
  PVRCLIENT_EVENT_SCHEDULE_CHANGE,
  PVRCLIENT_EVENT_DONE_RECORDING,
} PVR_EVENTS;

typedef struct {
  char *name;
  bool liveTV;
  bool canPause;
} PVRCLIENT_CAPABILITIES;

class IPVRClientCallback
{
public:
  virtual void OnMessage(DWORD clientID, int event, const std::string& data)=0;
};

class IPVRClient
{
public:
  IPVRClient(DWORD sourceID, IPVRClientCallback *callback){};
  virtual ~IPVRClient(){};

  /* server status */
  virtual PVRCLIENT_CAPABILITIES GetCapabilities()=0;
  virtual bool IsUp()=0;
  virtual bool GetDriveSpace(long long *total, long long *used)=0;

  /* channels */
  virtual int  GetNumChannels()=0;
  virtual void GetChannelList(CFileItemList &channels)=0;
  
  virtual bool GetEPGDataEnd(CDateTime &end)=0;
  virtual void GetEPGForChannel(int bouquet, int channel, CFileItemList &channelData)=0;

  /* scheduled recordings */
  virtual bool GetRecordingSchedules(CFileItemList &results)=0; // all the rec schedules, active or otherwise
  virtual bool GetUpcomingRecordings(CFileItemList &results)=0; // schedules that are currently active
  virtual bool GetConflicting(CFileItemList &results)=0; // schedules that are flagged as conflicting with each over

  /* recordings completed/started */
  virtual bool GetAllRecordings(CFileItemList &results)=0;

  /* individual programme operations */
  //virtual void UpdateRecStatus(CFileItem &programme)=0; // updates the recording status of this Fileitem (used in dialogs)

  /* livetv */

  /* ring buffer */

};
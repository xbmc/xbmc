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
#include "utils/EPG.h"

typedef enum {
  PVRCLIENT_EVENT_UNKNOWN = 20,
  PVRCLIENT_EVENT_CLOSE,
  PVRCLIENT_EVENT_RECORDING_LIST_CHANGE,
  PVRCLIENT_EVENT_SCHEDULE_CHANGE,
  PVRCLIENT_EVENT_DONE_RECORDING,
} PVRCLIENT_EVENTS;

typedef struct {
  char* Name;
  int   HasBouquets;
  char* DefaultHostname;
  int   DefaultPort;
  char* DefaultUser;
  char* DefaultPassword;
} PVRCLIENT_PROPS;

typedef struct {
  char* Name;
  char* Callsign;
  char* IconPath;
  int   Number;
} PVRCLIENT_CHANNEL;

typedef struct {
  int chanid;
  char callsign[30];
  char name[84];
  int sourceid;
  char title[150];
  char subtitle[150];
  char description[280];
  time_t starttime;
  time_t endtime;
  char episodeid[30];
  char seriesid[24];
  char category[84];
  int recording;
  int rec_status;
  int channum;
  int bouquet;
  int event_flags;
  int startoffset;
  int endoffset;
} PVRCLIENT_PROGRAMME;

typedef std::vector< PVRCLIENT_CHANNEL > PVR_CHANLIST;
typedef std::vector< PVRCLIENT_PROGRAMME > PVR_CHANDATA;

class IPVRClientCallback
{
public:
  virtual void OnClientMessage(DWORD clientID, int event, const std::string& data)=0;
  virtual void FillChannelData(DWORD clientID, PVRCLIENT_PROGRAMME* data, int count)=0;
};

class IPVRClient
{
public:
  IPVRClient(DWORD sourceID, IPVRClientCallback *callback){};
  virtual ~IPVRClient(){};

  /* server*/
  virtual DWORD GetID()=0;
  virtual void SetConnString(CURL connString)=0;
  virtual void Connect()=0;
  virtual PVRCLIENT_PROPS GetProperties()=0;
  virtual bool IsUp()=0;
  virtual bool GetDriveSpace(long long *total, long long *used)=0;

  /* bouquets */
  virtual int   GetBouquetForChannel(char* chanName)=0;
  virtual const char* GetBouquetName(int bouquetID)=0;
  virtual const char* GetBouquetIcon(int bouquetID)=0;
  virtual const char* GetBouquetGenre(int bouquetID)=0;

  /* channels */
  virtual int  GetNumChannels()=0;
  virtual int  GetChannelList(PVRCLIENT_CHANNEL* chanList)=0;
  
  virtual bool GetEPGDataEnd(CDateTime &end)=0;
  virtual void GetEPGForChannel(int bouquet, int channel)=0;

  /**
  * Get all timers, active or otherwise
  * \param results CFileItemList to be populated with timers
  * \return bool true if any timers found
  */
  virtual bool GetRecordingSchedules(CFileItemList* results)=0;

  /**
  * Get a list of scheduled recordings, including inactive
  * \param results CFileItemList to be populated with scheduled recordings
  * \return bool true if any scheduled recordings found
  */
  virtual bool GetUpcomingRecordings(CFileItemList* results)=0;

  /**
  * Get a list of any schedules that are flagged as conflicting
  * \param results CFileItemList to be populated with conflicting schedules
  * \return bool true if any conflicting schedules found
  */
  virtual bool GetConflicting(CFileItemList* results)=0;

  /**
  * Get a list of any recordings that are available, including ones not yet finished
  * \param results CFileItemList to be populated with list of recordings
  * \return bool true if any recordings found
  */
  virtual bool GetAllRecordings(CFileItemList* results)=0;

  /* individual programme operations */
  //virtual void UpdateRecStatus(CFileItem &programme)=0; // updates the recording status of this Fileitem (used in dialogs)

  /* livetv */

};

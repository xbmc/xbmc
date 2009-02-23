#pragma once

#ifndef __PVRCLIENT_VDR_H__
#define __PVRCLIENT_VDR_H__

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://xbmc.org
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

/*
* for DESCRIPTION see 'PVRClient-vdr.cpp'
*/

/* System includes */
#include <vector>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
/* XBMC includes */
#include "../../../pvrclients/PVRClientTypes.h"
#include "VTPSession.h"
//#include "include/CriticalSection.h"

class PVRClientVDR
{
public:
  /* Class interface */
  PVRClientVDR(PVRCallbacks *callbacks);
  ~PVRClientVDR();

  /* Server handling */
  PVR_ERROR GetProperties(PVR_SERVERPROPS *props);
  PVR_ERROR Connect();
  void Disconnect();
  bool IsUp();

  /* General handling */
  const char* GetBackendName();
  const char* GetBackendVersion();
  PVR_ERROR GetDriveSpace(long long *total, long long *used, int *percent);

  /* EPG handling */
  PVR_ERROR GetEPGForChannel(const unsigned int number, PVR_PROGLIST *epg, time_t start = NULL, time_t end = NULL);
  PVR_ERROR GetEPGNowInfo(const unsigned int number, PVR_PROGINFO *result);
  PVR_ERROR GetEPGNextInfo(const unsigned int number, PVR_PROGINFO *result);

  /* Channel handling */
  int GetNumChannels(void);
  PVR_ERROR GetAllChannels(PVR_CHANLIST* results, bool radio = false);
  /*PVR_ERROR GetChannelSettings(CTVChannelInfoTag *result);
  PVR_ERROR UpdateChannelSettings(const CTVChannelInfoTag &chaninfo);
  PVR_ERROR AddChannel(const CTVChannelInfoTag &info);
  PVR_ERROR DeleteChannel(unsigned int number);
  PVR_ERROR RenameChannel(unsigned int number, CStdString &newname);
  PVR_ERROR MoveChannel(unsigned int number, unsigned int newnumber);*/

  /* Record handling **/
  //int GetNumRecordings(void);
  //PVR_ERROR GetAllRecordings(VECRECORDINGS *results);
  //PVR_ERROR DeleteRecording(const CTVRecordingInfoTag &recinfo);
  //PVR_ERROR RenameRecording(const CTVRecordingInfoTag &recinfo, CStdString &newname);

  ///* Timer handling */
  //int GetNumTimers(void);
  //PVR_ERROR GetAllTimers(VECTVTIMERS *results);
  //PVR_ERROR GetTimerInfo(unsigned int timernumber, CTVTimerInfoTag &timerinfo);
  //PVR_ERROR AddTimer(const CTVTimerInfoTag &timerinfo);
  //PVR_ERROR DeleteTimer(const CTVTimerInfoTag &timerinfo, bool force = false);
  //PVR_ERROR RenameTimer(const CTVTimerInfoTag &timerinfo, CStdString &newname);
  //PVR_ERROR UpdateTimer(const CTVTimerInfoTag &timerinfo);

  ///* Live stream handling */
  //bool OpenLiveStream(unsigned int channel);
  //void CloseLiveStream();
  //int ReadLiveStream(BYTE* buf, int buf_size);
  //int GetCurrentClientChannel();
  //bool SwitchChannel(unsigned int channel);

  ///* Record stream handling */
  //bool OpenRecordedStream(const CTVRecordingInfoTag &recinfo);
  //void CloseRecordedStream(void);
  //int ReadRecordedStream(BYTE* buf, int buf_size);
  //__int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  //__int64 LengthRecordedStream(void);

protected:
  CVTPSession            *m_session;
  SOCKET                  m_socket;

private:
  long                    m_clientID;
  PVRCallbacks           *m_xbmc;
  int                     m_iCurrentChannel;
  bool                    m_bConnected;
  bool                    m_bCharsetIsUTF8;
  //CRITICAL_SECTION        m_critSection;

  /* Following is for recordings streams */
  uint64_t                currentPlayingRecordBytes;
  uint32_t                currentPlayingRecordFrames;
  uint64_t                currentPlayingRecordPosition;

  void Close();
};

#endif // __PVRCLIENT_VDR_H__

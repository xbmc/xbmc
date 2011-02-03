#pragma once
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
* for DESCRIPTION see 'PVRClient-MediaPortal.cpp'
*/

#include "os-dependent.h"

#include <vector>

/* Master defines for client control */
#ifndef _WINSOCKAPI_ //Prevent redefine warnings
#define _WINSOCKAPI_ //Needed here to prevent inclusion of <winsock.h> via the header below
#endif
#include "../../addons/include/xbmc_pvr_types.h"

/* Local includes */
#include "Socket.h"

class cPVRClientMediaPortal
{
public:
  /* Class interface */
  cPVRClientMediaPortal();
  ~cPVRClientMediaPortal();

  /* VTP Listening Thread */
  static void* Process(void*);

  /* Server handling */
  bool Connect();
  void Disconnect();
  bool IsUp();

  /* General handling */
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetMPTVTime(time_t *localTime, int *gmtOffset);

  /* EPG handling */
  PVR_ERROR RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start = NULL, time_t end = NULL);

  /* Channel handling */
  int GetNumChannels(void);
  PVR_ERROR RequestChannelList(PVRHANDLE handle, int radio = 0);

  /* Record handling **/
  int GetNumRecordings(void);
  PVR_ERROR RequestRecordingsList(PVRHANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname);

  /* Timer handling */
  int GetNumTimers(void);
  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force = false);
  PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);

  /* Live stream handling */
  bool OpenLiveStream(const PVR_CHANNEL &channelinfo);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char* buf, int buf_size);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo);

  /* Record stream handling */
  bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char* buf, int buf_size);
  long long SeekRecordedStream(long long pos, int whence=SEEK_SET);
  long long LengthRecordedStream(void);

  //MG: Added for MediaPortal streaming
  const char* GetLiveStreamURL(const PVR_CHANNEL &channelinfo);

protected:
  MPTV::Socket           *m_tcpclient;

private:
  bool GetChannel(unsigned int number, PVR_CHANNEL &channeldata);

  int                     m_iCurrentChannel;
  bool                    m_bConnected;
  bool                    m_bStop;
  bool                    m_bTimeShiftStarted;
  std::string             m_ConnectionString;
  std::string             m_BackendName;
  std::string             m_BackendVersion;
  time_t                  m_BackendUTCoffset;
  time_t                  m_BackendTime;

  void Close();

  //Used for TV Server communication:
  std::string SendCommand(std::string command);
  bool SendCommand2(std::string command, int& code, std::vector<std::string>& lines);
};

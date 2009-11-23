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

#include "vtptransceiver.h"
#include "StdString.h"
#include <vector>

/* Master defines for client control */
#include "../../addons/include/xbmc_pvr_types.h"

extern pthread_mutex_t m_critSection;

class cPVRClientVDR
{
public:
  /* Class interface */
  cPVRClientVDR();
  ~cPVRClientVDR();

  /* VTP Listening Thread */
  static void* Process(void*);

  /* Server handling */
  PVR_ERROR GetProperties(PVR_SERVERPROPS *props);

  bool Connect();
  void Disconnect();
  bool IsUp();

  /* General handling */
  const char* GetBackendName();
  const char* GetBackendVersion();
  const char* GetConnectionString();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetClientTime(time_t *time, int *diff_from_gmt);

  /* EPG handling */
  PVR_ERROR RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start = NULL, time_t end = NULL);

  /* Channel handling */
  int GetNumChannels(void);
  PVR_ERROR RequestChannelList(PVRHANDLE handle, bool radio = false);

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
  int ReadLiveStream(BYTE* buf, int buf_size);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channelinfo);
  PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo);

  /* Record stream handling */
  bool OpenRecordedStream(const PVR_RECORDINGINFO &recinfo);
  void CloseRecordedStream(void);
  int ReadRecordedStream(BYTE* buf, int buf_size);
  __int64 SeekRecordedStream(__int64 pos, int whence=SEEK_SET);
  __int64 LengthRecordedStream(void);

  static CVTPTransceiver *GetTransceiver() { return m_transceiver; }

protected:
  static CVTPTransceiver *m_transceiver;
  static SOCKET           m_socket_video;
  static SOCKET           m_socket_data;

private:
  /* VDR to XBMC Callback functions */
  static void* CallbackRcvThread(void* arg);
  bool VDRToXBMCCommand(char *Cmd);
  bool CallBackMODT(const char *Option);
  bool CallBackDELT(const char *Option);
  bool CallBackADDT(const char *Option);
  bool CallBackSMSG(const char *Option);
  bool CallBackIMSG(const char *Option);
  bool CallBackWMSG(const char *Option);
  bool CallBackEMSG(const char *Option);

  int                     m_iCurrentChannel;
  static bool             m_bConnected;
  pthread_t               m_thread;
  static bool             m_bStop;

  /* Following is for recordings streams */
  uint64_t                currentPlayingRecordBytes;
  uint32_t                currentPlayingRecordFrames;
  uint64_t                currentPlayingRecordPosition;

  CStdString              m_connectionString;

  void Close();
};

#endif // __PVRCLIENT_VDR_H__

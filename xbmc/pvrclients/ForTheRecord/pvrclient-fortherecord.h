#pragma once
/*
 *      Copyright (C) 2010-2011 Marcel Groothuis, Fred Hoogduin
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
#ifdef TSREADER
//#include "os-dependent.h"
#include "libPlatform/os-dependent.h"
#else
#include "libPlatform/os-dependent.h"
#endif 

#include <vector>

/* Master defines for client control */
#include "../../addons/include/xbmc_pvr_types.h"

#include "channel.h"
#include "recording.h"
#include "guideprogram.h"

#include "KeepAliveThread.h"
#ifdef TSREADER
//#include "lib/tsreader/TSReader.h"
class CTsReader;
#endif

#undef FTR_DUMPTS

class cPVRClientForTheRecord
{
public:
  /* Class interface */
  cPVRClientForTheRecord();
  ~cPVRClientForTheRecord();

  /* Server handling */
  bool Connect();
  void Disconnect();
  bool IsUp();
  bool ShareErrorsFound(void);

  /* General handling */
  const char* GetBackendName(void);
  const char* GetBackendVersion(void);
  const char* GetConnectionString(void);
  PVR_ERROR GetDriveSpace(long long *iTotal, long long *iUsed);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);

  /* EPG handling */
  PVR_ERROR GetEpg(PVR_HANDLE handle, const PVR_CHANNEL &channel, time_t iStart = NULL, time_t iEnd = NULL);

  /* Channel handling */
  int GetNumChannels(void);
  PVR_ERROR GetChannels(PVR_HANDLE handle, bool bRadio);
  /* Channel group handling */
  int GetChannelGroupsAmount(void);
  PVR_ERROR GetChannelGroups(PVR_HANDLE handle, bool bRadio);
  PVR_ERROR GetChannelGroupMembers(PVR_HANDLE handle, const PVR_CHANNEL_GROUP &group);

  /* Record handling **/
  int GetNumRecordings(void);
  PVR_ERROR GetRecordings(PVR_HANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDING &recinfo);

  /* Timer handling */
  int GetNumTimers(void);
  PVR_ERROR GetTimers(PVR_HANDLE handle);
  PVR_ERROR GetTimerInfo(unsigned int timernumber, PVR_TIMER &timer);
  PVR_ERROR AddTimer(const PVR_TIMER &timer);
  PVR_ERROR DeleteTimer(const PVR_TIMER &timer, bool bForceDelete = false);
  PVR_ERROR UpdateTimer(const PVR_TIMER &timer);

  /* Live stream handling */
  bool OpenLiveStream(const PVR_CHANNEL &channel);
  void CloseLiveStream();
  int ReadLiveStream(unsigned char *pBuffer, unsigned int iBufferSize);
  int GetCurrentClientChannel();
  bool SwitchChannel(const PVR_CHANNEL &channel);
  PVR_ERROR SignalStatus(PVR_SIGNAL_STATUS &signalStatus);

  /* Record stream handling */
  bool OpenRecordedStream(const PVR_RECORDING &recording);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekRecordedStream(long long iPosition, int iWhence = SEEK_SET);
  long long LengthRecordedStream(void);

  /* Used for rtsp streaming */
  const char* GetLiveStreamURL(const PVR_CHANNEL &channel);

private:
  cChannel* FetchChannel(int channel_uid);
  cChannel* FetchChannel(std::string channelid);
  void Close();
  bool FetchRecordingDetails(const Json::Value& data, cRecording& recording);
  bool FetchGuideProgramDetails(std::string Id, cGuideProgram& guideprogram);
  bool _OpenLiveStream(const PVR_CHANNEL &channel);

  int                     m_iCurrentChannel;
  bool                    m_bConnected;
  //bool                    m_bStop;
  bool                    m_bTimeShiftStarted;
  std::string             m_PlaybackURL;
  std::string             m_BackendName;
  int                     m_BackendVersion;
  time_t                  m_BackendUTCoffset;
  time_t                  m_BackendTime;

  std::vector<cChannel>   m_Channels; // Local channel cache list needed for id to guid conversion
  int                     m_channel_id_offset;
  int                     m_epg_id_offset;
  int                     m_signalqualityInterval;
//  CURL*                   m_curl;
#ifdef TSREADER
  CTsReader*              m_tsreader;
#endif //TSREADER
  CKeepAliveThread        m_keepalive;
#if defined(FTR_DUMPTS)
  char ofn[25];
  int ofd;
#endif

};

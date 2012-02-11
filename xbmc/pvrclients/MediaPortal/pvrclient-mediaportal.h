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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <vector>

/* Master defines for client control */
#include "../../addons/include/xbmc_pvr_types.h"

/* Local includes */
#include "Socket.h"
#include "Cards.h"
#include "epg.h"
#include "CriticalSection.h"

/* Use a forward declaration here. Including RTSPClient.h via TSReader.h at this point gives compile errors */
class CTsReader;

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
  PVR_ERROR DeleteRecording(const PVR_RECORDING &recording);
  PVR_ERROR RenameRecording(const PVR_RECORDING &recording);

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
  const char* GetLiveStreamURL(const PVR_CHANNEL &channel);

  /* Record stream handling */
  bool OpenRecordedStream(const PVR_RECORDING &recording);
  void CloseRecordedStream(void);
  int ReadRecordedStream(unsigned char *pBuffer, unsigned int iBufferSize);
  long long SeekRecordedStream(long long iPosition, int iWhence = SEEK_SET);
  long long LengthRecordedStream(void);

protected:
  MPTV::Socket           *m_tcpclient;

private:
  bool GetChannel(unsigned int number, PVR_CHANNEL &channeldata);
  bool LoadGenreXML(const std::string &filename);

  int                     m_iCurrentChannel;
  int                     m_iCurrentCard;
  bool                    m_bConnected;
  bool                    m_bStop;
  bool                    m_bTimeShiftStarted;
  std::string             m_ConnectionString;
  std::string             m_PlaybackURL;
  std::string             m_BackendName;
  std::string             m_BackendVersion;
  time_t                  m_BackendUTCoffset;
  time_t                  m_BackendTime;
  CCards                  m_cCards;
  GenreMap                m_genremap;
  CCriticalSection        m_mutex;
#ifdef TSREADER
  CTsReader*              m_tsreader;

  char                    m_noSignalStreamData[ 6 + 0xffff ];
  long                    m_noSignalStreamSize;
  long                    m_noSignalStreamReadPos;
  bool                    m_bPlayingNoSignal;
#endif //TSREADER
  void Close();

  //Used for TV Server communication:
  std::string SendCommand(std::string command);
  bool SendCommand2(std::string command, int& code, std::vector<std::string>& lines);
};

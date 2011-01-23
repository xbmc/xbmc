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

#include <vector>
#include "StdString.h"
#include "thread.h"
#include "tools.h"

#define CMD_LOCK cMutexLock CmdLock((cMutex*)&m_Mutex)

enum eSocketId {
  siLive,
  siReplay,
  siLiveFilter,
  siDataRespond,
  si_Count
};

class CVTPTransceiver
{
public:
  bool ReadResponse(int &code, std::string &line);
  bool ReadResponse(int &code, std::vector<std::string> &lines);

  bool SendCommand(const std::string &command);
  bool SendCommand(const std::string &command, int &code, std::string line);
  bool SendCommand(const std::string &command, int &code, std::vector<std::string> &lines);

private:
	SOCKET m_DataSockets[si_Count];
	SOCKET m_VTPSocket;
	cMutex m_Mutex;
	int    m_recIndex;

  struct sockaddr_in m_LocalAddr;
  struct sockaddr_in m_RemoteAddr;

  bool OpenStreamSocket(SOCKET& socket, struct sockaddr_in& address);
  bool AcceptStreamSocket(SOCKET& socket);
  bool Connect(const std::string &host, int port);
  void Close();
  bool IsConnected(SOCKET socket, fd_set *rd, fd_set *wr, fd_set *ex);
  void ScanVideoDir(PVRHANDLE handle, const char *DirName, bool Deleted = false, int LinkLevel = 0);

public:
  CVTPTransceiver();
  ~CVTPTransceiver();

  void Reset(void);

  bool IsOpen(void) const { return m_VTPSocket != INVALID_SOCKET; }
  bool CheckConnection();
  bool ProvidesChannel(unsigned int Channel, int Priority);
  bool CreateDataConnection(eSocketId Id);
  bool CloseDataConnection(eSocketId Id);
  SOCKET DataSocket(eSocketId Id) const { return m_DataSockets[Id]; }
  bool SetChannelDevice(unsigned int Channel);
  bool SetRecordingIndex(unsigned int Recording);
  bool GetPlayingRecordingSize(uint64_t *size, uint32_t *frames);
  uint64_t SeekRecordingPosition(uint64_t position);
  CStdString GetBackendName();
  CStdString GetBackendVersion();
  PVR_ERROR GetDriveSpace(long long *total, long long *used);
  PVR_ERROR GetBackendTime(time_t *localTime, int *gmtOffset);
  PVR_ERROR RequestEPGForChannel(const PVR_CHANNEL &channel, PVRHANDLE handle, time_t start = NULL, time_t end = NULL);
  int GetNumChannels(void);
  PVR_ERROR RequestChannelList(PVRHANDLE handle, bool radio = false);
  int GetNumRecordings(void);
  PVR_ERROR RequestRecordingsList(PVRHANDLE handle);
  PVR_ERROR DeleteRecording(const PVR_RECORDINGINFO &recinfo);
  PVR_ERROR RenameRecording(const PVR_RECORDINGINFO &recinfo, const char *newname);
  int GetNumTimers(void);
  PVR_ERROR RequestTimerList(PVRHANDLE handle);
  PVR_ERROR GetTimerInfo(unsigned int timernumber, PVR_TIMERINFO &tag);
  PVR_ERROR AddTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR DeleteTimer(const PVR_TIMERINFO &timerinfo, bool force = false);
  PVR_ERROR RenameTimer(const PVR_TIMERINFO &timerinfo, const char *newname);
  PVR_ERROR UpdateTimer(const PVR_TIMERINFO &timerinfo);
  PVR_ERROR SignalQuality(PVR_SIGNALQUALITY &qualityinfo, unsigned int channel);
  int TransferRecordingToSocket(uint64_t position, int size);
  bool SuspendServer(void);
  bool Quit(void);
};

extern class CVTPTransceiver VTPTransceiver;

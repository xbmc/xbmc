/*
 *      Copyright (C) 2010 Alwin Esch (Team XBMC)
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

#ifndef CMD_CONTROL_H
#define CMD_CONTROL_H

#include <queue>
#include <vdr/thread.h>

#include "responsepacket.h"
#include "requestpacket.h"

typedef std::queue<cRequestPacket*> RequestPacketQueue;

class cxSocket;

class cCmdControl : public cThread
{
public:
  cCmdControl();
  ~cCmdControl();

  bool init();
  bool recvRequest(cRequestPacket*);

private:
  bool processPacket();

  bool process_Login();
  bool process_GetTime();
  bool process_EnableStatusInterface();
  bool process_EnableOSDInterface();

  bool processRecStream_Open();
  bool processRecStream_Close();
  bool processRecStream_GetBlock();
  bool processRecStream_PositionFromFrameNumber();
  bool processRecStream_FrameNumberFromPosition();
  bool processRecStream_GetIFrame();

  bool processCHANNELS_GroupsCount();
  bool processCHANNELS_ChannelsCount();
  bool processCHANNELS_GroupList();
  bool processCHANNELS_GetChannels();

  bool processTIMER_GetCount();
  bool processTIMER_Get();
  bool processTIMER_GetList();
  bool processTIMER_Add();
  bool processTIMER_Delete();
  bool processTIMER_Update();

  bool processRECORDINGS_GetDiskSpace();
  bool processRECORDINGS_GetCount();
  bool processRECORDINGS_GetList();
  bool processRECORDINGS_GetInfo();
  bool processRECORDINGS_Delete();
  bool processRECORDINGS_Move();

  bool processEPG_GetForChannel();

  bool processSCAN_ScanSupported();
  bool processSCAN_GetCountries();
  bool processSCAN_GetSatellites();
  bool processSCAN_Start();
  bool processSCAN_Stop();

  /** Static callback functions to interact with wirbelscan plugin over
      the plugin service interface */
  static void processSCAN_AddCountry(int index, const char *isoName, const char *longName);
  static void processSCAN_AddSatellite(int index, const char *shortName, const char *longName);
  static void processSCAN_SetPercentage(int percent);
  static void processSCAN_SetSignalStrength(int strength, bool locked);
  static void processSCAN_SetDeviceInfo(const char *Info);
  static void processSCAN_SetTransponder(const char *Info);
  static void processSCAN_NewChannel(const char *Name, bool isRadio, bool isEncrypted, bool isHD);
  static void processSCAN_IsFinished();
  static void processSCAN_SetStatus(int status);
  static cResponsePacket *m_processSCAN_Response;
  static cxSocket *m_processSCAN_Socket;

  virtual void Action(void);

  cRequestPacket     *m_req;
  RequestPacketQueue  m_req_queue;
  cResponsePacket    *m_resp;
  cCondWait           m_Wait;
  cCharSetConv        m_toUTF8;
  cMutex              m_mutex;
};


#endif /* CMD_CONTROL_H */

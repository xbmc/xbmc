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

  virtual void Action(void);

  cRequestPacket     *req;
  RequestPacketQueue  req_queue;
  cResponsePacket    *resp;
  cCondWait           m_Wait;
};


#endif /* CMD_CONTROL_H */

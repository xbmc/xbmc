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

#ifndef CONNECTION_H
#define CONNECTION_H

#include <vdr/thread.h>
#include <vdr/receiver.h>
#include <vdr/status.h>

#include "config.h"
#include "cxsocket.h"
#include "cmdcontrol.h"

class cServer;
class cChannel;
class cDevice;
class cLiveStreamer;
class cResponsePacket;
class cRecPlayer;
class cCmdControl;

class cConnection : public cThread
                  , public cStatus
{
private:
  friend class cCmdControl;

  unsigned int    m_Id;
  cxSocket        m_socket;
  cServer        *m_server;
  bool            m_loggedIn;
  bool            m_StatusInterfaceEnabled;
  bool            m_OSDInterfaceEnabled;
  cLiveStreamer  *m_Streamer;
  const cChannel *m_Channel;
  bool            m_isStreaming;
  FILE           *m_NetLogFile;
  cString         m_ClientAddress;
  cRecPlayer     *m_RecPlayer;
  cCmdControl     m_cmdcontrol;

protected:
  virtual void Action(void);

  virtual void TimerChange(const cTimer *Timer, eTimerChange Change);
//  virtual void ChannelSwitch(const cDevice *Device, int ChannelNumber);
  virtual void Recording(const cDevice *Device, const char *Name, const char *FileName, bool On);
//  virtual void Replaying(const cControl *Control, const char *Name, const char *FileName, bool On);
//  virtual void SetVolume(int Volume, bool Absolute);
//  virtual void SetAudioTrack(int Index, const char * const *Tracks);
//  virtual void SetAudioChannel(int AudioChannel);
//  virtual void SetSubtitleTrack(int Index, const char * const *Tracks);
//  virtual void OsdClear(void);
//  virtual void OsdTitle(const char *Title);
  virtual void OsdStatusMessage(const char *Message);
//  virtual void OsdHelpKeys(const char *Red, const char *Green, const char *Yellow, const char *Blue);
//  virtual void OsdItem(const char *Text, int Index);
//  virtual void OsdCurrentItem(const char *Text);
//  virtual void OsdTextItem(const char *Text, bool Scroll);
//  virtual void OsdChannel(const char *Text);
//  virtual void OsdProgramme(time_t PresentTime, const char *PresentTitle, const char *PresentSubtitle, time_t FollowingTime, const char *FollowingTitle, const char *FollowingSubtitle);

public:
  cConnection(cServer *server, int fd, unsigned int id, const char *ClientAdr);
  virtual ~cConnection();

  unsigned int GetID() { return m_Id; }
  void SetLoggedIn(bool yesNo) { m_loggedIn = yesNo; }
  void SetStatusInterface(bool yesNo) { m_StatusInterfaceEnabled = yesNo; }
  void SetOSDInterface(bool yesNo) { m_OSDInterfaceEnabled = yesNo; }
  void EnableNetLog(bool yesNo, const char* ClientName = "");
  cxSocket *GetSocket() { return &m_socket; }
  bool StartChannelStreaming(const cChannel *channel, cResponsePacket *resp);
  void StopChannelStreaming();
  bool IsStreaming() { return m_isStreaming; }
  cRecPlayer *GetRecPlayer() { return m_RecPlayer; }
};

#endif /* CONNECTION_H */

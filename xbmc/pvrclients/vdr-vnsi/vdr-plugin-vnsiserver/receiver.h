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

#ifndef VNSIRECEIVER_H
#define VNSIRECEIVER_H

#include <linux/dvb/frontend.h>
#include <linux/videodev2.h>
#include <vdr/channels.h>
#include <vdr/device.h>
#include <vdr/receiver.h>
#include <vdr/thread.h>
#include <vdr/ringbuffer.h>

#include "demuxer.h"

class cxSocket;
class cChannel;
class cLiveReceiver;
class cTSDemuxer;
class cResponsePacket;

class cLiveStreamer : public cThread
                    , public cRingBufferLinear
{
private:
  friend class cParser;

  cDevice *GetDevice(const cChannel *Channel, int Priority);
  void Detach(void);
  void Attach(void);
  cTSDemuxer *FindStreamDemuxer(int Pid);

  void sendStreamPacket(sStreamPacket *pkt);
  void sendStreamChange();
  void sendSignalInfo();
  void sendStreamInfo();

  const cChannel   *m_Channel;                      /*!> Channel to stream */
  cDevice          *m_Device;                       /*!> The receiving device the channel depents to */
  cLiveReceiver    *m_Receiver;                     /*!> Our stream transceiver */
  int               m_Priority;                     /*!> The priority over other streamers */
  int               m_Pids[MAXRECEIVEPIDS + 1];     /*!> PID for cReceiver also as extra array */
  cTSDemuxer       *m_Streams[MAXRECEIVEPIDS + 1];  /*!> Stream information data (partly filled, rest is done by cLiveReceiver */
  int               m_NumStreams;                   /*!> Number of streams selected */
  cxSocket         *m_Socket;                       /*!> The socket class to communicate with client */
  int               m_Frontend;                     /*!> File descriptor to access used receiving device  */
  dvb_frontend_info m_FrontendInfo;                 /*!> DVB Information about the receiving device */
  v4l2_capability   m_vcap;
  cString           m_DeviceString;
  time_t            m_lastInfoSendet;               /*!> Last queue status report sent */
  bool              m_streamChangeSendet;
  bool              m_streamReady;
  bool              m_IsAudioOnly;
  bool              m_IsMPEGPS;

protected:
  virtual void Action(void);

public:
  cLiveStreamer();
  virtual ~cLiveStreamer();

  void Activate(bool On);

  bool StreamChannel(const cChannel *channel, int priority, cxSocket *Socket, cResponsePacket *resp);
  void SetReady() { m_streamReady = true; }
  bool IsReady() { return m_streamReady; }
  bool IsAudioOnly() { return m_IsAudioOnly; }
  bool IsMPEGPS() { return m_IsMPEGPS; }
};

#endif  /* VNSIRECEIVER_H */

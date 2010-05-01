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

#include <stdlib.h>
#include <sys/ioctl.h>

#include <vdr/remux.h>
#include <vdr/channels.h>

#include "config.h"
#include "receiver.h"
#include "cxsocket.h"
#include "vdrcommand.h"
#include "suspend.h"
#include "tools.h"
#include "responsepacket.h"

// --- cLiveReceiver -------------------------------------------------

class cLiveReceiver: public cReceiver
{
  friend class cLiveStreamer;

private:
  cLiveStreamer *m_Streamer;

protected:
  virtual void Activate(bool On);
  virtual void Receive(uchar *Data, int Length);

public:
  cLiveReceiver(cLiveStreamer *Streamer, tChannelID ChannelID, int Priority, const int *Pids);
  virtual ~cLiveReceiver();
};

cLiveReceiver::cLiveReceiver(cLiveStreamer *Streamer, tChannelID ChannelID, int Priority, const int *Pids)
 : cReceiver(ChannelID, Priority, 0, Pids)
 , m_Streamer(Streamer)
{
  LOGCONSOLE("Starting live receiver");
}

cLiveReceiver::~cLiveReceiver()
{
  LOGCONSOLE("Killing live receiver");
}

void cLiveReceiver::Receive(uchar *Data, int Length)
{
  int p = m_Streamer->Put(Data, Length);
  if (p != Length)
    m_Streamer->ReportOverflow(Length - p);
}

inline void cLiveReceiver::Activate(bool On)
{
  m_Streamer->Activate(On);
}


// --- cLiveStreamer -------------------------------------------------

cLiveStreamer::cLiveStreamer()
 : cThread("cLiveStreamer stream processor")
 , cRingBufferLinear(MEGABYTE(3), TS_SIZE, true)
{
  m_Channel         = NULL;
  m_Priority        = NULL;
  m_Socket          = NULL;
  m_Device          = NULL;
  m_Receiver        = NULL;
  m_Frontend        = -1;
  m_NumStreams      = 0;
  m_streamReady     = false;
  m_streamChangeSendet = false;
  m_lastInfoSendet  = time(NULL);
  memset(&m_FrontendInfo, 0, sizeof(m_FrontendInfo));
  for (int idx = 0; idx < MAXRECEIVEPIDS; ++idx)
  {
    m_Streams[idx] = NULL;
    m_Pids[idx]    = 0;
  }
}

cLiveStreamer::~cLiveStreamer()
{
  LOGCONSOLE("Started to delete live streamer");

  if (m_Device)
  {
    if (m_Receiver)
    {
      LOGCONSOLE("Detaching Live Receiver");
      m_Device->Detach(m_Receiver);
    }
    else
    {
      LOGCONSOLE("No live receiver present");
    }

    for (int idx = 0; idx < MAXRECEIVEPIDS; ++idx)
    {
      if (m_Streams[idx])
      {
        LOGCONSOLE("Deleting stream demuxer %i for pid=%i and type=%i", m_Streams[idx]->GetStreamID(), m_Streams[idx]->GetPID(), m_Streams[idx]->Type());
        DELETENULL(m_Streams[idx]);
        m_Streams[idx] = NULL;
        m_Pids[idx]    = 0;
      }
    }

    if (m_Receiver)
    {
      LOGCONSOLE("Deleting Live Receiver");
      DELETENULL(m_Receiver);
    }
  }
  if (m_Frontend >= 0)
  {
    close(m_Frontend);
    m_Frontend = -1;
  }
  LOGCONSOLE("Finished to delete live streamer");
}

void cLiveStreamer::Action(void)
{
  int readTimeouts      = 0;
  int processErrors     = 0;
  int signalInfoCnt     = 90;
  bool showingNoSignal  = false;
  bool recvRetry        = true;

  while (Running())
  {
    int size;
    int used = 0;
    unsigned char *buf = Get(size);
    if (buf)
    {
      /* Make sure we are looking at a TS packet */
      while (size > TS_SIZE)
      {
        if (buf[0] == TS_SYNC_BYTE && buf[TS_SIZE] == TS_SYNC_BYTE)
          break;
        buf++;
        size--;
        used++;
      }

      int processLoops = 0;
      while (size >= TS_SIZE)
      {
        unsigned int ts_pid = TsPid(buf+used);
        cTSDemuxer *demuxer = FindStreamDemuxer(ts_pid);
        if (demuxer)
        {
          if (!demuxer->ProcessTSPacket(buf+used))
            processErrors++;
        }

        size  -= TS_SIZE;
        used  += TS_SIZE;
        processLoops++;
      }
      Del(used);

      /* Additional Information and NO_SIGNAL timers */
      signalInfoCnt++;
      if (showingNoSignal && processErrors < processLoops/2)
      {
        readTimeouts = 0;
        m_streamChangeSendet = false;
        showingNoSignal = false;
      }
      else if (processErrors == 0)
        readTimeouts = 0;
    }


    if ((!buf && m_Receiver->IsAttached()) || processErrors > 0)
    {
      cCondWait::SleepMs(18);
      readTimeouts++;
      processErrors = 0;
      if (readTimeouts > 180 || showingNoSignal)
      {
        if (!showingNoSignal)
        {
          cResponsePacket *resp = new cResponsePacket();
          if (resp->initStream(VDR_STREAM_CHANGE, 0, 0, 0, 0))
          {
            resp->add_U32(1);
            resp->add_String("MPEG2VIDEO");
            resp->add_U32(0);
            resp->add_U32(0);
            resp->add_U32(720);
            resp->add_U32(576);
            resp->add_double(1.0);

            resp->finaliseStream();
            m_Socket->write(resp->getPtr(), resp->getLen());
          }
          else
          {
            esyslog("VNSI-Error: stream response packet init fail for NO_SIGNAL");
          }
          delete resp;
          isyslog("VNSI: No data in 3 seconds, queuing no signal image %i, %i", readTimeouts, showingNoSignal);
        }

        sStreamPacket pkt;
        pkt.id       = 0;
        pkt.data     = VNSIServerConfig.m_noSignalStreamData;
        pkt.size     = VNSIServerConfig.m_noSignalStreamSize;
        pkt.duration = 0;
        pkt.dts      = DVD_NOPTS_VALUE;
        pkt.pts      = DVD_NOPTS_VALUE;
        sendStreamPacket(&pkt);

        readTimeouts    = 0;
        showingNoSignal = true;
        recvRetry       = false;
      }
    }

    if (!m_Receiver->IsAttached()) /** Double check here */
    {
      if (!recvRetry)
      {
        isyslog("VNSI: returning from streamer thread, receiver is no more attached");
        return;
      }
      recvRetry = false;
      cCondWait::SleepMs(20);
    }

    if (time(NULL) - m_lastInfoSendet > 1)
    {
      m_lastInfoSendet = time(NULL);

      sendSignalInfo();
    }
    else if (signalInfoCnt >= 100 && !showingNoSignal)
    {
      /* Send stream information every 100 packets expect the first one is sendet
         after 10 packets */
      sendStreamInfo();
      signalInfoCnt = 0;
    }
  }
}

bool cLiveStreamer::StreamChannel(const cChannel *channel, int priority, cxSocket *Socket, cResponsePacket *resp)
{
  if (channel == NULL)
  {
    esyslog("VNSI-Error: Starting streaming of channel without valid channel");
    return false;
  }

  m_Channel   = channel;
  m_Priority  = priority;
  m_Socket    = Socket;
  m_Device    = GetDevice(channel, m_Priority);
  if (m_Device != NULL)
  {
    dsyslog("VNSI: Successfully found following device: %p (%d) for receiving", m_Device, m_Device ? m_Device->CardIndex() + 1 : 0);

    if (m_Device->SwitchChannel(m_Channel, false))
    {
      if (m_Channel->Vpid())
      {
#if APIVERSNUM >= 10701
        if (m_Channel->Vtype() == 0x1B)
          m_Streams[m_NumStreams] = new cTSDemuxer(this, m_NumStreams, stH264, m_Channel->Vpid());
        else
#endif
          m_Streams[m_NumStreams] = new cTSDemuxer(this, m_NumStreams, stMPEG2VIDEO, m_Channel->Vpid());

        m_Pids[m_NumStreams] = m_Channel->Vpid();
        m_NumStreams++;
      }

      const int *APids = m_Channel->Apids();
      for ( ; *APids && m_NumStreams < MAXRECEIVEPIDS; APids++)
      {
        int index = 0;
        if (!FindStreamDemuxer(*APids))
        {
          m_Pids[m_NumStreams]    = *APids;
          m_Streams[m_NumStreams] = new cTSDemuxer(this, m_NumStreams, stMPEG2AUDIO, *APids);
          m_Streams[m_NumStreams]->SetLanguage(m_Channel->Alang(index));
          m_NumStreams++;
        }
        index++;
      }

      const int *DPids = m_Channel->Dpids();
      for ( ; *DPids && m_NumStreams < MAXRECEIVEPIDS; DPids++)
      {
        int index = 0;
        if (!FindStreamDemuxer(*DPids))
        {
          m_Pids[m_NumStreams]    = *DPids;
          m_Streams[m_NumStreams] = new cTSDemuxer(this, m_NumStreams, stAC3, *DPids);
          m_Streams[m_NumStreams]->SetLanguage(m_Channel->Dlang(index));
          m_NumStreams++;
        }
        index++;
      }

      const int *SPids = m_Channel->Spids();
      if (SPids)
      {
        int index = 0;
        for ( ; *SPids && m_NumStreams < MAXRECEIVEPIDS; SPids++)
        {
          if (!FindStreamDemuxer(*SPids))
          {
            m_Pids[m_NumStreams]    = *SPids;
            m_Streams[m_NumStreams] = new cTSDemuxer(this, m_NumStreams, stDVBSUB, *SPids);
            m_Streams[m_NumStreams]->SetLanguage(m_Channel->Slang(index));
#if APIVERSNUM >= 10709
            m_Streams[m_NumStreams]->SetSubtitlingDescriptor(m_Channel->SubtitlingType(index),
                                                             m_Channel->CompositionPageId(index),
                                                             m_Channel->AncillaryPageId(index));
#endif
            m_NumStreams++;
          }
          index++;
        }
      }

      if (m_Channel->Tpid())
      {
        m_Streams[m_NumStreams] = new cTSDemuxer(this, m_NumStreams, stTELETEXT, m_Channel->Tpid());
        m_Pids[m_NumStreams]    = m_Channel->Tpid();
        m_NumStreams++;
      }

      m_Streams[m_NumStreams] = NULL;
      m_Pids[m_NumStreams]    = 0;

      /* Send the OK response here, that it is before the Stream end message */
      resp->add_U32(VDR_RET_OK);
      resp->finalise();
      m_Socket->write(resp->getPtr(), resp->getLen());

      if (m_NumStreams > 0 && m_Socket)
      {
        dsyslog("VNSI: Creating new live Receiver");
        m_Receiver = new cLiveReceiver(this, m_Channel->GetChannelID(), m_Priority, m_Pids);
        m_Device->AttachReceiver(m_Receiver);
      }

      isyslog("VNSI: Successfully switched to channel %i - %s", m_Channel->Number(), m_Channel->Name());
      return true;
    }
    else
    {
      dsyslog("VNSI: Can't switch to channel %i - %s", m_Channel->Number(), m_Channel->Name());
    }
  }
  else
  {
    esyslog("VNSI-Error: Can't get device for channel %i - %s", m_Channel->Number(), m_Channel->Name());
  }
  return false;
}

cTSDemuxer *cLiveStreamer::FindStreamDemuxer(int Pid)
{
  int idx;
  for (idx = 0; idx < m_NumStreams; ++idx)
    if (m_Streams[idx] && m_Streams[idx]->GetPID() == Pid)
      return m_Streams[idx];
  return NULL;
}

inline void cLiveStreamer::Activate(bool On)
{
  if (On)
  {
    LOGCONSOLE("VDR active, sending stream start message");
    Start();
  }
  else
  {
    LOGCONSOLE("VDR inactive, sending stream end message");
    Cancel();
  }
}

void cLiveStreamer::Attach(void)
{
  LOGCONSOLE("cLiveStreamer::Attach()");
  if (m_Device)
  {
    if (m_Receiver)
    {
      m_Device->Detach(m_Receiver);
      m_Device->AttachReceiver(m_Receiver);
    }
  }
}

void cLiveStreamer::Detach(void)
{
  LOGCONSOLE("cLiveStreamer::Detach()");
  if (m_Device)
  {
    if (m_Receiver)
      m_Device->Detach(m_Receiver);
  }
}

cDevice *cLiveStreamer::GetDevice(const cChannel *Channel, int Priority)
{
  cDevice *device = NULL;

  LOGCONSOLE("+ Statistics:");
  LOGCONSOLE("+ Current Channel: %d", cDevice::CurrentChannel());
  LOGCONSOLE("+ Current Device: %d", cDevice::ActualDevice()->CardIndex());
  LOGCONSOLE("+ Transfer Mode: %s", cDevice::ActualDevice() == cDevice::PrimaryDevice() ? "false" : "true");
  LOGCONSOLE("+ Replaying: %s", cDevice::PrimaryDevice()->Replaying() ? "true" : "false");
  LOGCONSOLE(" * GetDevice(const cChannel*, int)");
  LOGCONSOLE(" * -------------------------------");

  device = cDevice::GetDevice(Channel, Priority, false);

  LOGCONSOLE(" * Found following device: %p (%d)", device, device ? device->CardIndex() + 1 : 0);

#if CONSOLEDEBUG
  if (device == cDevice::ActualDevice())
  {
    LOGCONSOLE(" * is actual device");
  }
  if (!cSuspendCtl::IsActive() && VNSIServerConfig.SuspendMode != smAlways)
  {
    LOGCONSOLE(" * NOT suspended");
  }
#endif

  if (!device || (device == cDevice::ActualDevice()
                  && !cSuspendCtl::IsActive()
                  && VNSIServerConfig.SuspendMode != smAlways))
  {
    // mustn't switch actual device
    // maybe a device would be free if THIS connection did turn off its streams?
    LOGCONSOLE(" * trying again...");
    const cChannel *current = Channels.GetByNumber(cDevice::CurrentChannel());
    isyslog("VNSI: Detaching current receiver");
    Detach();
    device = cDevice::GetDevice(Channel, Priority, false);
    Attach();
    LOGCONSOLE(" * Found following device: %p (%d)", device, device ? device->CardIndex() + 1 : 0);

#if CONSOLEDEBUG
    if (device == cDevice::ActualDevice())
    {
      LOGCONSOLE(" * is actual device");
    }
    if (!cSuspendCtl::IsActive() && VNSIServerConfig.SuspendMode != smAlways)
    {
      LOGCONSOLE(" * NOT suspended");
    }
    if (current && !TRANSPONDER(Channel, current))
    {
      LOGCONSOLE(" * NOT same transponder");
    }
#endif

    if (device && (device == cDevice::ActualDevice()
                    && !cSuspendCtl::IsActive()
                    && VNSIServerConfig.SuspendMode != smAlways
                    && current != NULL
                    && !TRANSPONDER(Channel, current)))
    {
      // now we would have to switch away live tv...let's see if live tv
      // can be handled by another device
      cDevice *newdev = NULL;
      for (int i = 0; i < cDevice::NumDevices(); ++i)
      {
        cDevice *dev = cDevice::GetDevice(i);
        if (dev->ProvidesChannel(current, 0) && dev != device)
        {
          newdev = dev;
          break;
        }
      }
      LOGCONSOLE(" * Found device for live tv: %p (%d)", newdev, newdev ? newdev->CardIndex() + 1 : 0);
      if (newdev == NULL || newdev == device)
        // no suitable device to continue live TV, giving up...
        device = NULL;
      else
        newdev->SwitchChannel(current, true);
    }
  }

  return device;
}

void cLiveStreamer::sendStreamPacket(sStreamPacket *pkt)
{
  if (!m_streamChangeSendet)
  {
    sendStreamChange();
    m_streamChangeSendet = true;
  }

  if (pkt)
  {
#if 0
    LOGCONSOLE("sendet: %d %d %10lu %10lu %10d %10d", pkt->id, pkt->frametype, pkt->dts, pkt->pts, pkt->duration, pkt->size);
#endif
    uint32_t bufferLength = sizeof(uint32_t) * 5 + sizeof(int64_t) * 2;
    uint8_t buffer[bufferLength];
    *(uint32_t*)&buffer[0]  = htonl(CHANNEL_STREAM);        // stream channel
    *(uint32_t*)&buffer[4]  = htonl(VDR_STREAM_MUXPKT);     // Stream packet operation code
    *(uint32_t*)&buffer[8]  = htonl(pkt->id);               // Stream ID
    *(uint32_t*)&buffer[12] = htonl(pkt->duration);         // Duration
    *(int64_t*) &buffer[16] = htonll(pkt->pts);             // DTS
    *(int64_t*) &buffer[24] = htonll(pkt->dts);             // PTS
    *(uint32_t*)&buffer[32] = htonl(pkt->size);             // Data length
    m_Socket->write(&buffer, bufferLength);
    m_Socket->write(pkt->data, pkt->size);
  }
}

void cLiveStreamer::sendStreamChange()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VDR_STREAM_CHANGE, 0, 0, 0, 0))
  {
    esyslog("VNSI-Error: stream response packet init fail");
    delete resp;
    return;
  }

  for (int idx = 0; idx < m_NumStreams; ++idx)
  {
    if (m_Streams[idx])
    {
      resp->add_U32(m_Streams[idx]->GetStreamID());
      if (m_Streams[idx]->Type() == stMPEG2AUDIO)
      {
        resp->add_String("MPEG2AUDIO");
        resp->add_String(m_Streams[idx]->GetLanguage());
      }
      else if (m_Streams[idx]->Type() == stMPEG2VIDEO)
      {
        resp->add_String("MPEG2VIDEO");
        resp->add_U32(m_Streams[idx]->GetFpsScale());
        resp->add_U32(m_Streams[idx]->GetFpsRate());
        resp->add_U32(m_Streams[idx]->GetHeight());
        resp->add_U32(m_Streams[idx]->GetWidth());
        resp->add_double(m_Streams[idx]->GetAspect());
      }
      else if (m_Streams[idx]->Type() == stAC3)
      {
        resp->add_String("AC3");
        resp->add_String(m_Streams[idx]->GetLanguage());
      }
      else if (m_Streams[idx]->Type() == stH264)
      {
        resp->add_String("H264");
        resp->add_U32(m_Streams[idx]->GetFpsScale());
        resp->add_U32(m_Streams[idx]->GetFpsRate());
        resp->add_U32(m_Streams[idx]->GetHeight());
        resp->add_U32(m_Streams[idx]->GetWidth());
        resp->add_double(m_Streams[idx]->GetAspect());
      }
      else if (m_Streams[idx]->Type() == stDVBSUB)
      {
        resp->add_String("DVBSUB");
        resp->add_String(m_Streams[idx]->GetLanguage());
        resp->add_U32(m_Streams[idx]->CompositionPageId());
        resp->add_U32(m_Streams[idx]->AncillaryPageId());
      }
      else if (m_Streams[idx]->Type() == stTELETEXT)
        resp->add_String("TELETEXT");
      else if (m_Streams[idx]->Type() == stAAC)
      {
        resp->add_String("AAC");
        resp->add_String(m_Streams[idx]->GetLanguage());
      }
    }
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

void cLiveStreamer::sendSignalInfo()
{
  if (m_Frontend < 0)
  {
    cString dev = cString::sprintf(FRONTEND_DEVICE, m_Device->CardIndex(), 0);
    m_Frontend = open(dev, O_RDONLY | O_NONBLOCK);
    if (m_Frontend >= 0)
    {
      if (ioctl(m_Frontend, FE_GET_INFO, &m_FrontendInfo) < 0)
      {
        esyslog("VNSI-Error: cannot read frontend info.");
        close(m_Frontend);
        m_Frontend = -1;
        memset(&m_FrontendInfo, 0, sizeof(m_FrontendInfo));
        return;
      }
    }
  }
  else
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStream(VDR_STREAM_SIGNALINFO, 0, 0, 0, 0))
    {
      esyslog("VNSI-Error: stream response packet init fail");
      delete resp;
      return;
    }

    fe_status_t status;
    uint16_t fe_snr;
    uint16_t fe_signal;
    uint32_t fe_ber;
    uint32_t fe_unc;

    memset(&status, 0, sizeof(status));
    ioctl(m_Frontend, FE_READ_STATUS, &status);

    if (ioctl(m_Frontend, FE_READ_SIGNAL_STRENGTH, &fe_signal) == -1)
      fe_signal = -2;
    if (ioctl(m_Frontend, FE_READ_SNR, &fe_snr) == -1)
      fe_snr = -2;
    if (ioctl(m_Frontend, FE_READ_BER, &fe_ber) == -1)
      fe_ber = -2;
    if (ioctl(m_Frontend, FE_READ_UNCORRECTED_BLOCKS, &fe_unc) == -1)
      fe_unc = -2;

    switch (m_Channel->Source() & cSource::st_Mask)
    {
      case cSource::stSat:
        resp->add_String(*cString::sprintf("DVB-S%s #%d - %s", (m_FrontendInfo.caps & 0x10000000) ? "2" : "",  cDevice::ActualDevice()->CardIndex(), m_FrontendInfo.name));
        break;
      case cSource::stCable:
        resp->add_String(*cString::sprintf("DVB-C #%d - %s", cDevice::ActualDevice()->CardIndex(), m_FrontendInfo.name));
        break;
      case cSource::stTerr:
        resp->add_String(*cString::sprintf("DVB-T #%d - %s", cDevice::ActualDevice()->CardIndex(), m_FrontendInfo.name));
        break;
    }
    resp->add_String(*cString::sprintf("%s:%s:%s:%s:%s", (status & FE_HAS_LOCK) ? "LOCKED" : "-", (status & FE_HAS_SIGNAL) ? "SIGNAL" : "-", (status & FE_HAS_CARRIER) ? "CARRIER" : "-", (status & FE_HAS_VITERBI) ? "VITERBI" : "-", (status & FE_HAS_SYNC) ? "SYNC" : "-"));
    resp->add_U32(fe_snr);
    resp->add_U32(fe_signal);
    resp->add_U32(fe_ber);
    resp->add_U32(fe_unc);

    resp->finaliseStream();
    m_Socket->write(resp->getPtr(), resp->getLen());
    delete resp;
  }
}

void cLiveStreamer::sendStreamInfo()
{
  cResponsePacket *resp = new cResponsePacket();
  if (!resp->initStream(VDR_STREAM_CONTENTINFO, 0, 0, 0, 0))
  {
    esyslog("VNSI-Error: stream response packet init fail");
    delete resp;
    return;
  }

  for (int idx = 0; idx < m_NumStreams; ++idx)
  {
    if (m_Streams[idx])
    {
      if (m_Streams[idx]->Type() == stMPEG2AUDIO || m_Streams[idx]->Type() == stAC3 || m_Streams[idx]->Type() == stAAC)
      {
        resp->add_U32(m_Streams[idx]->GetStreamID());
        resp->add_String(m_Streams[idx]->GetLanguage());
        resp->add_U32(m_Streams[idx]->GetChannels());
        resp->add_U32(m_Streams[idx]->GetSampleRate());
        resp->add_U32(m_Streams[idx]->GetBlockAlign());
        resp->add_U32(m_Streams[idx]->GetBitRate());
        resp->add_U32(m_Streams[idx]->GetBitsPerSample());
      }
      else if (m_Streams[idx]->Type() == stMPEG2VIDEO || m_Streams[idx]->Type() == stH264)
      {
        resp->add_U32(m_Streams[idx]->GetStreamID());
        resp->add_U32(m_Streams[idx]->GetFpsScale());
        resp->add_U32(m_Streams[idx]->GetFpsRate());
        resp->add_U32(m_Streams[idx]->GetHeight());
        resp->add_U32(m_Streams[idx]->GetWidth());
        resp->add_double(m_Streams[idx]->GetAspect());
      }
      else if (m_Streams[idx]->Type() == stDVBSUB)
      {
        resp->add_U32(m_Streams[idx]->GetStreamID());
        resp->add_String(m_Streams[idx]->GetLanguage());
        resp->add_U32(m_Streams[idx]->CompositionPageId());
        resp->add_U32(m_Streams[idx]->AncillaryPageId());
      }
    }
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

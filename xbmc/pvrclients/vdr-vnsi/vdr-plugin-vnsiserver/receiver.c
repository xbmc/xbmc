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

#include <libsi/section.h>
#include <libsi/descriptor.h>

#include <vdr/remux.h>
#include <vdr/channels.h>

#include "config.h"
#include "receiver.h"
#include "cxsocket.h"
#include "vdrcommand.h"
#include "suspend.h"
#include "tools.h"
#include "responsepacket.h"

#if VDRVERSNUM < 10713
#ifndef PLUGINPARAMPATCHVERSNUM
#error "You must apply the pluginparam patch for VDR!"
#endif
#endif

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

// --- cLivePatFilter ----------------------------------------------------

class cLivePatFilter : public cFilter
{
private:
  int             m_pmtPid;
  int             m_pmtSid;
  int             m_pmtVersion;
  const cChannel *m_Channel;
  cLiveStreamer  *m_Streamer;

  int GetPid(SI::PMT::Stream& stream, eStreamType *type, char *langs, int *subtitlingType, int *compositionPageId, int *ancillaryPageId);
  void GetLanguage(SI::PMT::Stream& stream, char *langs);
  virtual void Process(u_short Pid, u_char Tid, const u_char *Data, int Length);

public:
  cLivePatFilter(cLiveStreamer *Streamer, const cChannel *Channel);
};

cLivePatFilter::cLivePatFilter(cLiveStreamer *Streamer, const cChannel *Channel)
{
  LOGCONSOLE("cStreamdevPatFilter(\"%s\")", Channel->Name());
  m_Channel     = Channel;
  m_Streamer    = Streamer;
  m_pmtPid      = 0;
  m_pmtSid      = 0;
  m_pmtVersion  = -1;
  Set(0x00, 0x00);  // PAT

}

static const char * const psStreamTypes[] = {
        "UNKNOWN",
        "ISO/IEC 11172 Video",
        "ISO/IEC 13818-2 Video",
        "ISO/IEC 11172 Audio",
        "ISO/IEC 13818-3 Audio",
        "ISO/IEC 13818-1 Privete sections",
        "ISO/IEC 13818-1 Private PES data",
        "ISO/IEC 13512 MHEG",
        "ISO/IEC 13818-1 Annex A DSM CC",
        "0x09",
        "ISO/IEC 13818-6 Multiprotocol encapsulation",
        "ISO/IEC 13818-6 DSM-CC U-N Messages",
        "ISO/IEC 13818-6 Stream Descriptors",
        "ISO/IEC 13818-6 Sections (any type, including private data)",
        "ISO/IEC 13818-1 auxiliary",
        "ISO/IEC 13818-7 Audio with ADTS transport sytax",
        "ISO/IEC 14496-2 Visual (MPEG-4)",
        "ISO/IEC 14496-3 Audio with LATM transport syntax",
        "0x12", "0x13", "0x14", "0x15", "0x16", "0x17", "0x18", "0x19", "0x1a",
        "ISO/IEC 14496-10 Video (MPEG-4 part 10/AVC, aka H.264)",
        "",
};

void cLivePatFilter::GetLanguage(SI::PMT::Stream& stream, char *langs)
{
  SI::Descriptor *d;
  for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
  {
    switch (d->getDescriptorTag())
    {
      case SI::ISO639LanguageDescriptorTag:
      {
        SI::ISO639LanguageDescriptor *ld = (SI::ISO639LanguageDescriptor *)d;
        strn0cpy(langs, I18nNormalizeLanguageCode(ld->languageCode), MAXLANGCODE1);
        break;
      }
      default: ;
    }
    delete d;
  }
}

int cLivePatFilter::GetPid(SI::PMT::Stream& stream, eStreamType *type, char *langs, int *subtitlingType, int *compositionPageId, int *ancillaryPageId)
{
  SI::Descriptor *d;
  *langs = 0;

  if (!stream.getPid())
    return 0;

  switch (stream.getStreamType())
  {
    case 0x01: // ISO/IEC 11172 Video
    case 0x02: // ISO/IEC 13818-2 Video
      LOGCONSOLE("cStreamdevPatFilter PMT scanner adding PID %d (%s)\n", stream.getPid(), psStreamTypes[stream.getStreamType()]);
      *type = stMPEG2VIDEO;
      return stream.getPid();
    case 0x03: // ISO/IEC 11172 Audio
    case 0x04: // ISO/IEC 13818-3 Audio
      *type   = stMPEG2AUDIO;
      GetLanguage(stream, langs);
      LOGCONSOLE("cStreamdevPatFilter PMT scanner adding PID %d (%s) (%s)\n", stream.getPid(), psStreamTypes[stream.getStreamType()], langs);
      return stream.getPid();
#if 1
    case 0x07: // ISO/IEC 13512 MHEG
    case 0x08: // ISO/IEC 13818-1 Annex A  DSM CC
    case 0x0a: // ISO/IEC 13818-6 Multiprotocol encapsulation
    case 0x0b: // ISO/IEC 13818-6 DSM-CC U-N Messages
    case 0x0c: // ISO/IEC 13818-6 Stream Descriptors
    case 0x0d: // ISO/IEC 13818-6 Sections (any type, including private data)
    case 0x0e: // ISO/IEC 13818-1 auxiliary
#endif
    case 0x0f: // ISO/IEC 13818-7 Audio with ADTS transport syntax
    case 0x10: // ISO/IEC 14496-2 Visual (MPEG-4)
    case 0x11: // ISO/IEC 14496-3 Audio with LATM transport syntax
      LOGCONSOLE("cStreamdevPatFilter PMT scanner: Not adding PID %d (%s) (skipped)\n", stream.getPid(), psStreamTypes[stream.getStreamType()]);
      break;
    case 0x1b: // ISO/IEC 14496-10 Video (MPEG-4 part 10/AVC, aka H.264)
      LOGCONSOLE("cStreamdevPatFilter PMT scanner adding PID %d (%s)\n", stream.getPid(), psStreamTypes[stream.getStreamType()]);
      *type = stH264;
      return stream.getPid();
    case 0x05: // ISO/IEC 13818-1 private sections
    case 0x06: // ISO/IEC 13818-1 PES packets containing private data
      for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
      {
        switch (d->getDescriptorTag())
        {
          case SI::AC3DescriptorTag:
            LOGCONSOLE("cStreamdevPatFilter PMT scanner: adding PID %d (%s) %s (%s)\n", stream.getPid(), psStreamTypes[stream.getStreamType()], "AC3", langs);
            *type = stAC3;
            GetLanguage(stream, langs);
            delete d;
            return stream.getPid();
          case SI::EnhancedAC3DescriptorTag:
            LOGCONSOLE("cStreamdevPatFilter PMT scanner: adding PID %d (%s) %s (%s)\n", stream.getPid(), psStreamTypes[stream.getStreamType()], "EAC3", langs);
            *type = stEAC3;
            GetLanguage(stream, langs);
            delete d;
            return stream.getPid();
          case SI::DTSDescriptorTag:
            LOGCONSOLE("cStreamdevPatFilter PMT scanner: adding PID %d (%s) %s (%s)\n", stream.getPid(), psStreamTypes[stream.getStreamType()], "DTS", langs);
            *type = stDTS;
            GetLanguage(stream, langs);
            delete d;
            return stream.getPid();
          case SI::AACDescriptorTag:
            LOGCONSOLE("cStreamdevPatFilter PMT scanner: adding PID %d (%s) %s (%s)\n", stream.getPid(), psStreamTypes[stream.getStreamType()], "AAC", langs);
            *type = stAAC;
            GetLanguage(stream, langs);
            delete d;
            return stream.getPid();
          case SI::TeletextDescriptorTag:
            LOGCONSOLE("cStreamdevPatFilter PMT scanner: adding PID %d (%s) %s\n", stream.getPid(), psStreamTypes[stream.getStreamType()], "Teletext");
            *type = stTELETEXT;
            delete d;
            return stream.getPid();
          case SI::SubtitlingDescriptorTag:
          {
            *type               = stDVBSUB;
            *langs              = 0;
            *subtitlingType     = 0;
            *compositionPageId  = 0;
            *ancillaryPageId    = 0;
            SI::SubtitlingDescriptor *sd = (SI::SubtitlingDescriptor *)d;
            SI::SubtitlingDescriptor::Subtitling sub;
            char *s = langs;
            int n = 0;
            for (SI::Loop::Iterator it; sd->subtitlingLoop.getNext(sub, it); )
            {
              if (sub.languageCode[0])
              {
                *subtitlingType     = sub.getSubtitlingType();
                *compositionPageId  = sub.getCompositionPageId();
                *ancillaryPageId    = sub.getAncillaryPageId();
                if (n > 0)
                  *s++ = '+';
                strn0cpy(s, I18nNormalizeLanguageCode(sub.languageCode), MAXLANGCODE1);
                s += strlen(s);
                if (n++ > 1)
                  break;
              }
            }
            delete d;
            LOGCONSOLE("cStreamdevPatFilter PMT scanner: adding PID %d (%s) %s\n", stream.getPid(), psStreamTypes[stream.getStreamType()], "DVBSUB");
            return stream.getPid();
          }
          default:
            LOGCONSOLE("cStreamdevPatFilter PMT scanner: NOT adding PID %d (%s) %s (%i)\n", stream.getPid(), psStreamTypes[stream.getStreamType()], "UNKNOWN", d->getDescriptorTag());
            break;
        }
        delete d;
      }
      break;
    default:
      /* This following section handles all the cases where the audio track
       * info is stored in PMT user info with stream id >= 0x80
       * we check the registration format identifier to see if it
       * holds "AC-3"
       */
      if (stream.getStreamType() >= 0x80)
      {
        bool found = false;
        for (SI::Loop::Iterator it; (d = stream.streamDescriptors.getNext(it)); )
        {
          switch (d->getDescriptorTag())
          {
            case SI::RegistrationDescriptorTag:
            /* unfortunately libsi does not implement RegistrationDescriptor */
            if (d->getLength() >= 4)
            {
              found = true;
              SI::CharArray rawdata = d->getData();
              if (/*rawdata[0] == 5 && rawdata[1] >= 4 && */
                  rawdata[2] == 'A' && rawdata[3] == 'C' &&
                  rawdata[4] == '-' && rawdata[5] == '3')
              {
                LOGCONSOLE("cStreamdevPatFilter PMT scanner: Adding pid %d (type 0x%x) RegDesc len %d (%c%c%c%c)\n",
                            stream.getPid(), stream.getStreamType(), d->getLength(), rawdata[2], rawdata[3], rawdata[4], rawdata[5]);
                *type = stAC3;
                delete d;
                return stream.getPid();
              }
            }
            break;
            default:
            break;
          }
          delete d;
        }
        if (!found)
        {
          LOGCONSOLE("Adding pid %d (type 0x%x) RegDesc not found -> assume AC-3\n", stream.getPid(), stream.getStreamType());
          *type = stAC3;
          return stream.getPid();
        }
      }
      LOGCONSOLE("cStreamdevPatFilter PMT scanner: NOT adding PID %d (%s) %s\n", stream.getPid(), psStreamTypes[stream.getStreamType()<0x1c?stream.getStreamType():0], "UNKNOWN");
      break;
  }
  *type = stNone;
  return 0;
}

void cLivePatFilter::Process(u_short Pid, u_char Tid, const u_char *Data, int Length)
{
  if (Pid == 0x00)
  {
    if (Tid == 0x00)
    {
      SI::PAT pat(Data, false);
      if (!pat.CheckCRCAndParse())
        return;
      SI::PAT::Association assoc;
      for (SI::Loop::Iterator it; pat.associationLoop.getNext(assoc, it); )
      {
        if (!assoc.isNITPid())
        {
          const cChannel *Channel =  Channels.GetByServiceID(Source(), Transponder(), assoc.getServiceId());
          if (Channel && (Channel == m_Channel))
          {
            int prevPmtPid = m_pmtPid;
            if (0 != (m_pmtPid = assoc.getPid()))
            {
              m_pmtSid = assoc.getServiceId();
              if (m_pmtPid != prevPmtPid)
              {
                Add(m_pmtPid, 0x02);
                m_pmtVersion = -1;
              }
              return;
            }
          }
        }
      }
    }
  }
  else if (Pid == m_pmtPid && Tid == SI::TableIdPMT && Source() && Transponder())
  {
    SI::PMT pmt(Data, false);
    if (!pmt.CheckCRCAndParse())
      return;
    if (pmt.getServiceId() != m_pmtSid)
      return; // skip broken PMT records
    if (m_pmtVersion != -1)
    {
      if (m_pmtVersion != pmt.getVersionNumber())
      {
//        printf("cStreamdevPatFilter: PMT version changed, detaching all pids\n");
        cFilter::Del(m_pmtPid, 0x02);
        m_pmtPid = 0; // this triggers PAT scan
      }
      return;
    }
    m_pmtVersion = pmt.getVersionNumber();

    SI::PMT::Stream stream;
    int         pids[MAXRECEIVEPIDS + 1];
    eStreamType types[MAXRECEIVEPIDS + 1];
    char        langs[MAXRECEIVEPIDS + 1][MAXLANGCODE2];
    int         subtitlingType[MAXRECEIVEPIDS + 1];
    int         compositionPageId[MAXRECEIVEPIDS + 1];
    int         ancillaryPageId[MAXRECEIVEPIDS + 1];
    int         streams = 0;
    for (SI::Loop::Iterator it; pmt.streamLoop.getNext(stream, it); )
    {
      eStreamType type;
      int pid = GetPid(stream, &type, langs[streams], &subtitlingType[streams], &compositionPageId[streams], &ancillaryPageId[streams]);
      if (0 != pid && streams < MAXRECEIVEPIDS)
      {
        pids[streams]   = pid;
        types[streams]  = type;
        streams++;
      }
    }
    pids[streams] = 0;

    int newstreams = 0;
    for (int i = 0; i < streams; i++)
    {
      if (m_Streamer->HaveStreamDemuxer(pids[i], types[i]) == -1)
        newstreams++;
    }

    if (newstreams > 0)
    {
      if (m_Streamer->m_Receiver)
      {
        LOGCONSOLE("Detaching Live Receiver");
        m_Streamer->m_Device->Detach(m_Streamer->m_Receiver);
        DELETENULL(m_Streamer->m_Receiver);
      }

      for (int idx = 0; idx < MAXRECEIVEPIDS; ++idx)
      {
        if (m_Streamer->m_Streams[idx])
        {
          DELETENULL(m_Streamer->m_Streams[idx]);
          m_Streamer->m_Pids[idx] = 0;
        }
      }
      m_Streamer->m_NumStreams  = 0;
      m_Streamer->m_streamReady = true;

      for (int i = 0; i < streams; i++)
      {
        switch (types[i])
        {
          case stMPEG2AUDIO:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stMPEG2AUDIO, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stMPEG2VIDEO:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stMPEG2VIDEO, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stH264:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stH264, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stAC3:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stAC3, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stEAC3:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stEAC3, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stDTS:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stDTS, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stAAC:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stAAC, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stDVBSUB:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stDVBSUB, pids[i]);
            m_Streamer->m_Streams[m_Streamer->m_NumStreams]->SetLanguage(langs[i]);
            m_Streamer->m_Streams[m_Streamer->m_NumStreams]->SetSubtitlingDescriptor(subtitlingType[i], compositionPageId[i], ancillaryPageId[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
          case stTELETEXT:
          {
            m_Streamer->m_Streams[m_Streamer->m_NumStreams] = new cTSDemuxer(m_Streamer, m_Streamer->m_NumStreams, stTELETEXT, pids[i]);
            m_Streamer->m_Pids[m_Streamer->m_NumStreams] = pids[i];
            m_Streamer->m_NumStreams++;
            break;
          }
        }
      }

      m_Streamer->m_Receiver  = new cLiveReceiver(m_Streamer, m_Channel->GetChannelID(), m_Streamer->m_Priority, m_Streamer->m_Pids);
      m_Streamer->m_Device->AttachReceiver(m_Streamer->m_Receiver);
      isyslog("VNSI: Currently unknown new streams found, receiver and demuxers reinited\n");
    }
  }
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
  m_PatFilter       = NULL;
  m_Frontend        = -1;
  m_NumStreams      = 0;
  m_streamReady     = false;
  m_IsAudioOnly     = false;
  m_IsMPEGPS        = false;
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

    if (m_PatFilter)
    {
      LOGCONSOLE("Detaching Live Filter");
      m_Device->Detach(m_PatFilter);
    }
    else
    {
      LOGCONSOLE("No live filter present");
    }

    for (int idx = 0; idx < MAXRECEIVEPIDS; ++idx)
    {
      if (m_Streams[idx])
      {
        LOGCONSOLE("Deleting stream demuxer %i for pid=%i and type=%i", m_Streams[idx]->GetStreamID(), m_Streams[idx]->GetPID(), m_Streams[idx]->Type());
        DELETENULL(m_Streams[idx]);
        m_Pids[idx] = 0;
      }
    }

    if (m_Receiver)
    {
      LOGCONSOLE("Deleting Live Receiver");
      DELETENULL(m_Receiver);
    }

    if (m_PatFilter)
    {
      LOGCONSOLE("Deleting Live Filter");
      DELETENULL(m_PatFilter);
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
  int signalInfoCnt     = 90;
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

      while (size >= TS_SIZE)
      {
        if (!Running())
          break;

        unsigned int ts_pid = TsPid(buf+used);
        cTSDemuxer *demuxer = FindStreamDemuxer(ts_pid);

        if (demuxer)
          demuxer->ProcessTSPacket(buf+used);

        size  -= TS_SIZE;
        used  += TS_SIZE;
      }
      Del(used);

      signalInfoCnt++;
      readTimeouts = 0;
    }

    if (!buf && m_Receiver->IsAttached())
    {
      cCondWait::SleepMs(18);
      readTimeouts++;
      if (readTimeouts > 180)
      {

        uint8_t dummyData[1];
        sStreamPacket pkt;
        pkt.id       = 0;
        pkt.data     = dummyData;
        pkt.size     = 0;
        pkt.duration = 0;
        pkt.dts      = DVD_NOPTS_VALUE;
        pkt.pts      = DVD_NOPTS_VALUE;
        sendStreamPacket(&pkt);

        readTimeouts = 0;
      }
    }

    if (!Running())
      break;

    if (!m_Receiver->IsAttached()) /** Double check here */
    {
      isyslog("VNSI: returning from streamer thread, receiver is no more attached");
      break;
    }

    if (time(NULL) - m_lastInfoSendet > 1)
    {
      m_lastInfoSendet = time(NULL);

      sendSignalInfo();
    }
    else if (signalInfoCnt >= 100)
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
      else
      {
        /* m_streamReady is set by the Video demuxers, to have always valid stream informations
         * like height and width. But if no Video PID is present like for radio channels
         * VNSI will deadlock
         */
        m_streamReady = true;
        m_IsAudioOnly = true;
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

#if VDRVERSNUM < 10713
      if (m_Channel && m_Channel->IsPlug()) m_IsMPEGPS = true;
#else
      // TODO: rework cPvrSourceParams for use with older vdr
      if (m_Channel && ((m_Channel->Source() >> 24) == 'V')) m_IsMPEGPS = true;
#endif

      if (m_NumStreams > 0 && m_Socket)
      {
        dsyslog("VNSI: Creating new live Receiver");
        m_Receiver  = new cLiveReceiver(this, m_Channel->GetChannelID(), m_Priority, m_Pids);
        m_PatFilter = new cLivePatFilter(this, m_Channel);
        m_Device->AttachReceiver(m_Receiver);
        m_Device->AttachFilter(m_PatFilter);
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

int cLiveStreamer::HaveStreamDemuxer(int Pid, eStreamType streamType)
{
  int idx;
  for (idx = 0; idx < m_NumStreams; ++idx)
    if (m_Streams[idx] && (Pid == 0 || m_Streams[idx]->GetPID() == Pid) && m_Streams[idx]->Type() == streamType)
      return idx;
  return -1;
}

inline void cLiveStreamer::Activate(bool On)
{
  if (On)
  {
    LOGCONSOLE("VDR active, sending stream start message");
    m_streamChangeSendet = false;
    Start();
  }
  else
  {
    LOGCONSOLE("VDR inactive, sending stream end message");
    Cancel(1);
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
      else if (m_Streams[idx]->Type() == stEAC3)
      {
        resp->add_String("EAC3");
        resp->add_String(m_Streams[idx]->GetLanguage());
      }
      else if (m_Streams[idx]->Type() == stDTS)
      {
        resp->add_String("DTS");
        resp->add_String(m_Streams[idx]->GetLanguage());
      }
    }
  }

  resp->finaliseStream();
  m_Socket->write(resp->getPtr(), resp->getLen());
  delete resp;
}

#define MINSIGNALSTRENGTH       16383
void cLiveStreamer::sendSignalInfo()
{
  /* If no frontend is found m_Frontend is set to -2, in this case
     return a empty signalinfo package */
  if (m_Frontend == -2)
  {
    cResponsePacket *resp = new cResponsePacket();
    if (!resp->initStream(VDR_STREAM_SIGNALINFO, 0, 0, 0, 0))
    {
      esyslog("VNSI-Error: stream response packet init fail");
      delete resp;
      return;
    }

    resp->add_String(*cString::sprintf("Unknown"));
    resp->add_String(*cString::sprintf("Unknown"));
    resp->add_U32(0);
    resp->add_U32(0);
    resp->add_U32(0);
    resp->add_U32(0);

    resp->finaliseStream();
    m_Socket->write(resp->getPtr(), resp->getLen());
    delete resp;
    return;
  }

#if VDRVERSNUM < 10713
  if (m_Channel && m_Channel->IsPlug())
#else
  // TODO: rework cPvrSourceParams for use with older vdr
  if (m_Channel && ((m_Channel->Source() >> 24) == 'V'))
#endif
  {
    struct v4l2_tuner tuner;
    if (m_Frontend < 0)
    {
      for (int i = 0; i < 8; i++)
      {
        m_DeviceString = cString::sprintf("/dev/video%d", i);
        m_Frontend = open(m_DeviceString, O_RDONLY | O_NONBLOCK);
        if (m_Frontend >= 0)
        {
          if (ioctl(m_Frontend, VIDIOC_QUERYCAP, &m_vcap) < 0)
          {
            esyslog("VNSI-Error: cannot read analog frontend info.");
            close(m_Frontend);
            m_Frontend = -1;
            memset(&m_vcap, 0, sizeof(m_vcap));
            continue;
          }
          break;
        }
      }
      if (m_Frontend < 0)
        m_Frontend = -2;
    }

    if (m_Frontend >= 0)
    {
      cResponsePacket *resp = new cResponsePacket();
      if (!resp->initStream(VDR_STREAM_SIGNALINFO, 0, 0, 0, 0))
      {
        esyslog("VNSI-Error: stream response packet init fail");
        delete resp;
        return;
      }
/*
      memset(&tuner, 0, sizeof(tuner));
      tuner.index = 0;
      tuner.type  = V4L2_TUNER_ANALOG_TV;

      if (ioctl(m_Frontend, VIDIOC_G_TUNER, &tuner) == 0)
      {
        int timeout = 1000;
        while (timeout > 0)
        {
          cCondWait::SleepMs(10);
          timeout -= 10;
          ioctl(m_Frontend, VIDIOC_G_TUNER, &tuner);
          if (tuner.signal > MINSIGNALSTRENGTH)
          {
            break;
          }
        }
      }
*/
      resp->add_String(*cString::sprintf("Analog #%s - %s (%s)", *m_DeviceString, (char *) m_vcap.card, m_vcap.driver));
//      resp->add_String(*cString::sprintf("%s", (tuner.signal > MINSIGNALSTRENGTH) ? "LOCKED" : "-"));
      resp->add_String("");
      resp->add_U32(0);
      resp->add_U32(0);
//      resp->add_U32(tuner.signal);
      resp->add_U32(0);
      resp->add_U32(0);

      resp->finaliseStream();
      m_Socket->write(resp->getPtr(), resp->getLen());
      delete resp;
    }
  }
  else
  {
    if (m_Frontend < 0)
    {
      m_DeviceString = cString::sprintf(FRONTEND_DEVICE, m_Device->CardIndex(), 0);
      m_Frontend = open(m_DeviceString, O_RDONLY | O_NONBLOCK);
      if (m_Frontend >= 0)
      {
        if (ioctl(m_Frontend, FE_GET_INFO, &m_FrontendInfo) < 0)
        {
          esyslog("VNSI-Error: cannot read frontend info.");
          close(m_Frontend);
          m_Frontend = -2;
          memset(&m_FrontendInfo, 0, sizeof(m_FrontendInfo));
          return;
        }
      }
    }

    if (m_Frontend >= 0)
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
      if (m_Streams[idx]->Type() == stMPEG2AUDIO ||
          m_Streams[idx]->Type() == stAC3 ||
          m_Streams[idx]->Type() == stEAC3 ||
          m_Streams[idx]->Type() == stDTS ||
          m_Streams[idx]->Type() == stAAC)
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

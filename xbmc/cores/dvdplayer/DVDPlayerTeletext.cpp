/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "stdafx.h"
#include "Settings.h"
#include "DVDPlayer.h"
#include "DVDPlayerTeletext.h"
#include "Application.h"
#include "hamm.h"

using namespace std;

uint8_t invtab[256] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};


cTelePage::cTelePage(PageID t_page, unsigned char t_flags, unsigned char t_lang, int t_mag)
  : mag(t_mag), flags(t_flags), lang(t_lang), page(t_page)
{
   memset(pagebuf,' ',26*40);
}

cTelePage::~cTelePage()
{
}

void cTelePage::SetLine(int line, unsigned char *myptr)
{
   memcpy(pagebuf+40*line,myptr,40);
}

CDVDTeletextData::CDVDTeletextData()
: CThread()
, m_messageQueue("teletext")
, m_TxtPage(NULL)
{
  m_speed = DVD_PLAYSPEED_NORMAL;

  InitializeCriticalSection(&m_critSection);
  m_messageQueue.SetMaxDataSize(40 * 256 * 1024);
}

CDVDTeletextData::~CDVDTeletextData()
{
  StopThread();
  delete m_TxtPage;
  DeleteCriticalSection(&m_critSection);
}

bool CDVDTeletextData::CheckStream(CDVDStreamInfo &hints, int type)
{
  if (hints.codec == CODEC_ID_EBU_TELETEXT && type == DVDPLAYERDATA_TELETEXT)
    return true;

  return false;
}

bool CDVDTeletextData::OpenStream(CDVDStreamInfo &hints, int type)
{
  m_messageQueue.Init();
  
  if (hints.codec == CODEC_ID_EBU_TELETEXT && type == DVDPLAYERDATA_TELETEXT)
  {
    CLog::Log(LOGNOTICE, "Creating teletext data thread");
    Create();
    return true;
  }
  
  return false;
}

void CDVDTeletextData::CloseStream(bool bWaitForBuffers)
{
  // wait until buffers are empty
  if (bWaitForBuffers && m_speed > 0) m_messageQueue.WaitUntilEmpty();

  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for data thread to exit");

  StopThread(); // will set this->m_bStop to true

  m_messageQueue.End();

  ResetTeletextCache();
}

int CDVDTeletextData::GetTeletextPageCount()
{
  return m_pageStorage.size();
}

void CDVDTeletextData::ResetTeletextCache()
{
  if (m_pageStorage.empty())
    return;

  map<long, cTelePage*>::iterator itr = m_pageStorage.begin();
  while (itr != m_pageStorage.end())
  {
    delete m_pageStorage[(*itr).first];
    itr++;
  }
  m_pageStorage.clear();

  delete m_TxtPage;
  m_TxtPage = NULL;
}

bool CDVDTeletextData::GetTeletextPagePresent(int Page, int subPage)
{
  EnterCriticalSection(&m_critSection);

  map<long, cTelePage*>::iterator itr;

  long id = subPage << 16 | Page;
  itr = m_pageStorage.find(id);

  if (itr != m_pageStorage.end())
  {
    LeaveCriticalSection(&m_critSection);
    return true;
  }
  
  LeaveCriticalSection(&m_critSection);
  return false;
}

bool CDVDTeletextData::GetTeletextPage(int Page, int subPage, BYTE* buf)
{
  EnterCriticalSection(&m_critSection);

  map<long, cTelePage*>::iterator itr;

  long id = subPage << 16 | Page;
  itr = m_pageStorage.find(id);

  if (itr != m_pageStorage.end())
  {
    //   0     String "VTXV4"
    //   5     always 0x01
    //   6     magazine number
    //   7     page number
    //   8     flags
    //   9     lang
    //   10    always 0x00
    //   11    always 0x00
    //   12    teletext data, 40x24 bytes
    memcpy(buf, "VTXV4", 5);
    buf[5]  = 0x01;
    buf[6]  = m_pageStorage[(*itr).first]->mag;
    buf[7]  = m_pageStorage[(*itr).first]->page.page;
    buf[8]  = m_pageStorage[(*itr).first]->flags;
    buf[9]  = m_pageStorage[(*itr).first]->lang;
    buf[10] = 0x00;
    buf[11] = 0x00;
    memcpy(&buf[12], m_pageStorage[(*itr).first]->pagebuf, 24*40);

    LeaveCriticalSection(&m_critSection);
    return true;
  }
  
  LeaveCriticalSection(&m_critSection);
  return false;
}
  
void CDVDTeletextData::OnStartup()
{
  CThread::SetName("CDVDTeletextData");
}

void CDVDTeletextData::Process()
{
  CLog::Log(LOGNOTICE, "running thread: data_thread");

  while (!m_bStop)
  {
    CDVDMsg* pMsg;
    int iPriority = (m_speed == DVD_PLAYSPEED_PAUSE) ? 1 : 0;
    MsgQueueReturnCode ret = m_messageQueue.Get(&pMsg, 2000, iPriority);

    if (ret == MSGQ_TIMEOUT)
    {
      /* Timeout for Teletext is not a bad thing, so we continue without error */
      continue;
    }

    if (MSGQ_IS_ERROR(ret) || ret == MSGQ_ABORT)
    {
      CLog::Log(LOGERROR, "Got MSGQ_ABORT or MSGO_IS_ERROR return true (%i)", ret);
      break;
    }

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      EnterCriticalSection(&m_critSection);

      DemuxPacket* pPacket = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
      bool bPacketDrop     = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacketDrop();
      uint8_t *Datai       = pPacket->pData;
      int pages            = (pPacket->iSize - 1) / 46;
  
      /* Is it a ITU-R System B Teletext stream in acc. to EN 300 472 */
      if (Datai[0] >= 0x10 && Datai[0] <= 0x1F) /* data_identifier */
      {
        Datai -= 3;
        for (int i=0; i < pages; i++)
        {
          if (Datai[4+i*46]==2 || Datai[4+i*46]==3)
          {
            for (int j=(8+i*46);j<(50+i*46);j++)
              Datai[j]=invtab[Datai[j]];
            
            DecodeTXT(&Datai[i*46+8]);
          }
        }
      }

      LeaveCriticalSection(&m_critSection);
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)) // private message sent by (CDVDTeletextData::Flush())
    {
      EnterCriticalSection(&m_critSection);
      ResetTeletextCache();
      LeaveCriticalSection(&m_critSection);
    }
    pMsg->Release();
  }
}

void CDVDTeletextData::OnExit()
{
  CLog::Log(LOGNOTICE, "thread end: data_thread");
}

void CDVDTeletextData::Flush()
{
  /* flush using message as this get's called from dvdplayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
}

void CDVDTeletextData::DecodeTXT(unsigned char* ptr)
{
   // Format of buffer:
   //   0x00-0x04  ?
   //   0x05-0x06  Clock Run-In?
   //   0x07       Framing Code?
   //   0x08       Magazine number (100-digit of page number)
   //   0x09       Line number
   //   0x0A..0x31 Line data
   // Line 0 only:
   //   0x0A       10-digit of page number
   //   0x0B       1-digit of page number
   //   0x0C       Sub-Code bits 0..3
   //   0x0D       Sub-Code bits 4..6 + C4 flag
   //   0x0E       Sub-Code bits 8..11
   //   0x0F       Sub-Code bits 12..13 + C5,C6 flag
   //   0x10       C7-C10 flags
   //   0x11       C11-C14 flags
   //
   // Flags:
   //   C4 - Erase last page, new page transmitted
   //   C5 - News flash, boxed display
   //   C6 - Subtitle, boxed display
   //   C7 - Suppress Header, dont show line 0
   //   C8 - Update, page has changed
   //   C9 - Interrupt Sequence, page number is out of order
   //   C10 - Inhibit Display
   //   C11 - Magazine Serial mode
   //   C12-C14 - Language selection, lower 3 bits


  int hdr,mag,mag8,line;
  unsigned char flags,lang;
  int err = 0;

  hdr = hamm16(ptr, &err);
  if (err & 0xf000)
    return;

  mag   = hdr & 7;
  mag8  = mag ?: 8;
  line  = (hdr >> 3) & 0x1f;
  ptr  += 2;

  if (line == 0)
  {
    int b1, b2, b3, b4;
    int pgno, subno;

    b1 = hamm16(ptr, &err);       // page number
    b2 = hamm16(ptr+2, &err);     // subpage number + flags
    b3 = hamm16(ptr+4, &err);     // subpage number + flags
    b4 = hamm16(ptr+6, &err);     // language code + more flags

    if (b1 == 0xff) return;
    
    if (m_TxtPage)
    {
      map<long, cTelePage*>::iterator itr;
    
      long id = m_TxtPage->page.subPage << 16 | m_TxtPage->page.page;
      itr = m_pageStorage.find(id);
      if (itr != m_pageStorage.end())
      {
        delete m_pageStorage[(*itr).first];
        m_pageStorage[(*itr).first] = m_TxtPage;
      }
      else
      {
        m_pageStorage.insert(std::make_pair(id, m_TxtPage));
      }
      m_TxtPage = NULL;
    }
    
    // flags:
    //   0x80  C4 - Erase page
    //   0x40  C5 - News flash
    //   0x20  C6 - Subtitle
    //   0x10  C7 - Suppress Header
    //   0x08  C8 - Update
    //   0x04  C9 - Interrupt Sequence
    //   0x02  C9 (Bug?)
    //   0x01  C11 - Magazine Serial mode
    flags  =b2 & 0x80;
    flags |=(b3&0x40)|((b3>>2)&0x20); //??????
    flags |=((b4<<4)&0x10)|((b4<<2)&0x08)|(b4&0x04)|((b4>>1)&0x02)|((b4>>4)&0x01);
    lang   =((b4>>5) & 0x07);
    pgno   = mag8 * 256 + b1;
    subno  = (b2 + b3 * 256) & 0x3f7f;
    
    m_TxtPage = new cTelePage(PageID(pgno, subno), flags, lang, mag);
    m_TxtPage->SetLine((int)line,(unsigned char *)ptr);
  }
  else if (line >= 1 && line <= 25)
  {
    if (m_TxtPage)
      m_TxtPage->SetLine((int)line,(unsigned char *)ptr); 
  }
}

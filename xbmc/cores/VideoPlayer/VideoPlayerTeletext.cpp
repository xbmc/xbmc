/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "VideoPlayerTeletext.h"
#include "TimingConstants.h"
#include "DVDStreamInfo.h"
#include "DVDDemuxers/DVDDemuxPacket.h"
#include "utils/log.h"
#include "threads/SingleLock.h"

const uint8_t rev_lut[32] =
{
  0x00,0x08,0x04,0x0c, /*  upper nibble */
  0x02,0x0a,0x06,0x0e,
  0x01,0x09,0x05,0x0d,
  0x03,0x0b,0x07,0x0f,
  0x00,0x80,0x40,0xc0, /*  lower nibble */
  0x20,0xa0,0x60,0xe0,
  0x10,0x90,0x50,0xd0,
  0x30,0xb0,0x70,0xf0
};

void CDVDTeletextTools::NextDec(int *i) /* skip to next decimal */
{
  (*i)++;

  if ((*i & 0x0F) > 0x09)
    *i += 0x06;

  if ((*i & 0xF0) > 0x90)
    *i += 0x60;

  if (*i > 0x899)
    *i = 0x100;
}

void CDVDTeletextTools::PrevDec(int *i)           /* counting down */
{
  (*i)--;

  if ((*i & 0x0F) > 0x09)
    *i -= 0x06;

  if ((*i & 0xF0) > 0x90)
    *i -= 0x60;

  if (*i < 0x100)
    *i = 0x899;
}

/* print hex-number into string, s points to last digit, caller has to provide enough space, no termination */
void CDVDTeletextTools::Hex2Str(char *s, unsigned int n)
{
  do {
    char c = (n & 0xF);
    *s-- = number2char(c);
    n >>= 4;
  } while (n);
}

signed int CDVDTeletextTools::deh24(unsigned char *p)
{
  int e = hamm24par[0][p[0]]
    ^ hamm24par[1][p[1]]
    ^ hamm24par[2][p[2]];

  int x = hamm24val[p[0]]
    + (p[1] & 127) * 16
    + (p[2] & 127) * 2048;

  return (x ^ hamm24cor[e]) | hamm24err[e];
}


CDVDTeletextData::CDVDTeletextData(CProcessInfo &processInfo)
: CThread("DVDTeletextData")
, IDVDStreamPlayer(processInfo)
, m_messageQueue("teletext")
{
  m_speed = DVD_PLAYSPEED_NORMAL;

  m_messageQueue.SetMaxDataSize(40 * 256 * 1024);

  /* Initialize Data structures */
  memset(&m_TXTCache.astCachetable, 0,    sizeof(m_TXTCache.astCachetable));
  memset(&m_TXTCache.astP29,        0,    sizeof(m_TXTCache.astP29));
  ResetTeletextCache();
}

CDVDTeletextData::~CDVDTeletextData()
{
  StopThread();
  ResetTeletextCache();
}

bool CDVDTeletextData::CheckStream(CDVDStreamInfo &hints)
{
  if (hints.codec == AV_CODEC_ID_DVB_TELETEXT)
    return true;

  return false;
}

bool CDVDTeletextData::OpenStream(CDVDStreamInfo &hints)
{
  CloseStream(true);

  m_messageQueue.Init();

  if (hints.codec == AV_CODEC_ID_DVB_TELETEXT)
  {
    CLog::Log(LOGNOTICE, "Creating teletext data thread");
    Create();
    return true;
  }

  return false;
}

void CDVDTeletextData::CloseStream(bool bWaitForBuffers)
{
  m_messageQueue.Abort();

  // wait for decode_video thread to end
  CLog::Log(LOGNOTICE, "waiting for teletext data thread to exit");

  StopThread(); // will set this->m_bStop to true

  m_messageQueue.End();
  ResetTeletextCache();
}


void CDVDTeletextData::ResetTeletextCache()
{
  CSingleLock lock(m_critSection);

  /* Reset Data structures */
  for (int i = 0; i < 0x900; i++)
  {
    for (int j = 0; j < 0x80; j++)
    {
      if (m_TXTCache.astCachetable[i][j])
      {
        TextPageinfo_t *p = &(m_TXTCache.astCachetable[i][j]->pageinfo);
        if (p->p24)
          free(p->p24);

        if (p->ext)
        {
          if (p->ext->p27)
            free(p->ext->p27);

          for (int d26 = 0; d26 < 16; d26++)
          {
            if (p->ext->p26[d26])
              free(p->ext->p26[d26]);
          }
          free(p->ext);
        }
        delete m_TXTCache.astCachetable[i][j];
        m_TXTCache.astCachetable[i][j] = 0;
      }
    }
  }

  for (int i = 0; i < 9; i++)
  {
    if (m_TXTCache.astP29[i])
    {
      if (m_TXTCache.astP29[i]->p27)
        free(m_TXTCache.astP29[i]->p27);

      for (int d26 = 0; d26 < 16; d26++)
      {
        if (m_TXTCache.astP29[i]->p26[d26])
          free(m_TXTCache.astP29[i]->p26[d26]);
      }
      free(m_TXTCache.astP29[i]);
      m_TXTCache.astP29[i] = 0;
    }
    m_TXTCache.CurrentPage[i]    = -1;
    m_TXTCache.CurrentSubPage[i] = -1;
  }

  memset(&m_TXTCache.SubPageTable,  0xFF, sizeof(m_TXTCache.SubPageTable));
  memset(&m_TXTCache.astP29,        0,    sizeof(m_TXTCache.astP29));
  memset(&m_TXTCache.BasicTop,      0,    sizeof(m_TXTCache.BasicTop));
  memset(&m_TXTCache.ADIPTable,     0,    sizeof(m_TXTCache.ADIPTable));
  memset(&m_TXTCache.FlofPages,     0,    sizeof(m_TXTCache.FlofPages));
  memset(&m_TXTCache.SubtitlePages, 0,    sizeof(m_TXTCache.SubtitlePages));
  memset(&m_TXTCache.astCachetable, 0,    sizeof(m_TXTCache.astCachetable));
  memset(&m_TXTCache.TimeString,    0x20, 8);

  m_TXTCache.NationalSubset           = NAT_DEFAULT;/* default */
  m_TXTCache.NationalSubsetSecondary  = NAT_DEFAULT;
  m_TXTCache.ZapSubpageManual         = false;
  m_TXTCache.PageUpdate               = false;
  m_TXTCache.ADIP_PgMax               = -1;
  m_TXTCache.BTTok                    = false;
  m_TXTCache.CachedPages              = 0;
  m_TXTCache.PageReceiving            = -1;
  m_TXTCache.Page                     = 0x100;
  m_TXTCache.SubPage                  = m_TXTCache.SubPageTable[m_TXTCache.Page];
  m_TXTCache.line30                   = "";
  if (m_TXTCache.SubPage == 0xff)
    m_TXTCache.SubPage = 0;
}

void CDVDTeletextData::Process()
{
  int             b1, b2, b3, b4;
  int             packet_number;
  TextPageinfo_t *pageinfo_thread;
  unsigned char   vtxt_row[42];
  unsigned char   pagedata[9][23*40];
  unsigned char   magazine  = 0xff;
//  int             doupdate  = 0;

  CLog::Log(LOGNOTICE, "running thread: CDVDTeletextData");

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

    if (MSGQ_IS_ERROR(ret))
    {
      CLog::Log(LOGERROR, "Got MSGQ_ABORT or MSGO_IS_ERROR return true (%i)", ret);
      break;
    }

    if (pMsg->IsType(CDVDMsg::DEMUXER_PACKET))
    {
      CSingleLock lock(m_critSection);

      DemuxPacket* pPacket = ((CDVDMsgDemuxerPacket*)pMsg)->GetPacket();
      uint8_t *Datai       = pPacket->pData;
      int rows             = (pPacket->iSize - 1) / 46;

      /* Is it a ITU-R System B Teletext stream in acc. to EN 300 472 */
      if (Datai[0] >= 0x10 && Datai[0] <= 0x1F) /* Check we have a valid data identifier */
      {
        /* Go thru the pages stored inside this frame */
        for (int row=0; row < rows; row++)
        {
          uint8_t *vtx_rowbyte  = &Datai[(row*46)+1];

          /* Check for valid data_unit_id */
          if ((vtx_rowbyte[0] == 0x02 || vtx_rowbyte[0] == 0x03) && (vtx_rowbyte[1] == 0x2C))
          {
            /* clear rowbuffer */
            /* convert row from lsb to msb (begin with magazine number) */
            for (int i = 4; i < 46; i++)
            {
              uint8_t upper = (vtx_rowbyte[i] >> 4) & 0xf;
              uint8_t lower = vtx_rowbyte[i] & 0xf;
              vtxt_row[i-4] = (rev_lut[upper]) | (rev_lut[lower+16]);
            }

            /* get packet number */
            b1 = dehamming[vtxt_row[0]];
            b2 = dehamming[vtxt_row[1]];

            if (b1 == 0xFF || b2 == 0xFF)
              continue;

            b1 &= 8;

            /* get packet and magazine number */
            packet_number = b1>>3 | b2<<1;
            magazine      = dehamming[vtxt_row[0]] & 7;
            if (!magazine) magazine = 8;

            if (packet_number == 0 && m_TXTCache.CurrentPage[magazine] != -1 && m_TXTCache.CurrentSubPage[magazine] != -1)
              SavePage(m_TXTCache.CurrentPage[magazine], m_TXTCache.CurrentSubPage[magazine], pagedata[magazine]);

            /* analyze row */
            if (packet_number == 0)
            {
              /* get pagenumber */
              b2 = dehamming[vtxt_row[3]];
              b3 = dehamming[vtxt_row[2]];

              if (b2 == 0xFF || b3 == 0xFF)
              {
                m_TXTCache.CurrentPage[magazine] = m_TXTCache.PageReceiving = -1;
                continue;
              }

              m_TXTCache.CurrentPage[magazine] = m_TXTCache.PageReceiving = magazine<<8 | b2<<4 | b3;

              if (b2 == 0x0f && b3 == 0x0f)
              {
                m_TXTCache.CurrentSubPage[magazine] = -1; /* ?ff: ignore data transmissions */
                continue;
              }

              /* get subpagenumber */
              b1 = dehamming[vtxt_row[7]];
              b2 = dehamming[vtxt_row[6]];
              b3 = dehamming[vtxt_row[5]];
              b4 = dehamming[vtxt_row[4]];

              if (b1 == 0xFF || b2 == 0xFF || b3 == 0xFF || b4 == 0xFF)
              {
                m_TXTCache.CurrentSubPage[magazine] = -1;
                continue;
              }

              b1 &= 3;
              b3 &= 7;

              if (IsDec(m_TXTCache.PageReceiving)) /* ignore other subpage bits for hex pages */
                m_TXTCache.CurrentSubPage[magazine] = b3<<4 | b4;
              else
                m_TXTCache.CurrentSubPage[magazine] = b4; /* max 16 subpages for hex pages */

              /* store current subpage for this page */
              m_TXTCache.SubPageTable[m_TXTCache.CurrentPage[magazine]] = m_TXTCache.CurrentSubPage[magazine];

              AllocateCache(magazine);
              LoadPage(m_TXTCache.CurrentPage[magazine], m_TXTCache.CurrentSubPage[magazine], pagedata[magazine]);
              pageinfo_thread = &(m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]]->pageinfo);
              if (!pageinfo_thread)
                continue;

              if ((m_TXTCache.PageReceiving & 0xff) == 0xfe) /* ?fe: magazine organization table (MOT) */
                pageinfo_thread->function = FUNC_MOT;

              /* check controlbits */
              if (dehamming[vtxt_row[5]] & 8)   /* C4 -> erase page */
              {
                memset(m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]]->data, ' ', 23*40);
                memset(pagedata[magazine],' ', 23*40);
              }
//              if (dehamming[vtxt_row[9]] & 8)   /* C8 -> update page */
//                doupdate = m_TXTCache.PageReceiving;

              pageinfo_thread->boxed = !!(dehamming[vtxt_row[7]] & 0x0c);

              /* get country control bits */
              b1 = dehamming[vtxt_row[9]];
              if (b1 != 0xFF)
              {
                pageinfo_thread->nationalvalid = 1;
                pageinfo_thread->national = rev_lut[b1] & 0x07;
              }

              if (dehamming[vtxt_row[7]] & 0x08)// subtitle page
              {
                int i = 0, found = -1, use = -1;
                for (; i < 8; i++)
                {
                  if (use == -1 && !m_TXTCache.SubtitlePages[i].page)
                    use = i;
                  else if (m_TXTCache.SubtitlePages[i].page == m_TXTCache.PageReceiving)
                  {
                    found = i;
                    use = i;
                    break;
                  }
                }
                if (found == -1 && use != -1)
                  m_TXTCache.SubtitlePages[use].page = m_TXTCache.PageReceiving;
                if (use != -1)
                  m_TXTCache.SubtitlePages[use].language = CountryConversionTable[pageinfo_thread->national];
              }

              /* check parity, copy line 0 to cache (start and end 8 bytes are not needed and used otherwise) */
              unsigned char *p = m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]]->p0;
              for (int i = 10; i < 42-8; i++)
                *p++ = deparity[vtxt_row[i]];

              if (!IsDec(m_TXTCache.PageReceiving))
                continue; /* valid hex page number: just copy headline, ignore TimeString */

              /* copy TimeString */
              p = m_TXTCache.TimeString;
              for (int i = 42-8; i < 42; i++)
                *p++ = deparity[vtxt_row[i]];
            }
            else if (packet_number == 29 && dehamming[vtxt_row[2]]== 0) /* packet 29/0 replaces 28/0 for a whole magazine */
            {
              Decode_p2829(vtxt_row, &(m_TXTCache.astP29[magazine]));
            }
            else if (m_TXTCache.CurrentPage[magazine] != -1 && m_TXTCache.CurrentSubPage[magazine] != -1)
              /* packet>0, 0 has been correctly received, buffer allocated */
            {
              pageinfo_thread = &(m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]]->pageinfo);
              if (!pageinfo_thread)
                continue;

              /* pointer to current info struct */
              if (packet_number <= 25)
              {
                unsigned char *p = NULL;
                if (packet_number < 24)
                {
                  p = pagedata[magazine] + 40*(packet_number-1);
                }
                else
                {
                  if (!(pageinfo_thread->p24))
                    pageinfo_thread->p24 = (unsigned char*) calloc(2, 40);
                  if (pageinfo_thread->p24)
                    p = pageinfo_thread->p24 + (packet_number - 24) * 40;
                }
                if (p)
                {
                  if (IsDec(m_TXTCache.CurrentPage[magazine]))
                  {
                    for (int i = 2; i < 42; i++)
                    {
                      *p++ = vtxt_row[i] & 0x7f; /* allow values with parity errors as some channels don't care :( */
                    }
                  }
                  else if ((m_TXTCache.CurrentPage[magazine] & 0xff) == 0xfe)
                  {
                    for (int i = 2; i < 42; i++)
                    {
                      *p++ = dehamming[vtxt_row[i]]; /* decode hamming 8/4 */
                    }
                  }
                  else /* other hex page: no parity check, just copy */
                    memcpy(p, &vtxt_row[2], 40);
                }
              }
              else if (packet_number == 27)
              {
                int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */
                if (descode == 0xff)
                  continue;

                if (descode == 0) // reading FLOF-Pagelinks
                {
                  b1 = dehamming[vtxt_row[0]];
                  if (b1 != 0xff)
                  {
                    b1 &= 7;

                    for (int i = 0; i < FLOFSIZE; i++)
                    {
                      b2 = dehamming[vtxt_row[4+i*6]];
                      b3 = dehamming[vtxt_row[3+i*6]];

                      if (b2 != 0xff && b3 != 0xff)
                      {
                        b4 = ((b1 ^ (dehamming[vtxt_row[8+i*6]]>>1)) & 6) | ((b1 ^ (dehamming[vtxt_row[6+i*6]]>>3)) & 1);
                        if (b4 == 0)
                          b4 = 8;
                        if (b2 <= 9 && b3 <= 9)
                          m_TXTCache.FlofPages[m_TXTCache.CurrentPage[magazine] ][i] = b4<<8 | b2<<4 | b3;
                      }
                    }

                    /* copy last 2 links to ADIPTable for TOP-Index */
                    if (pageinfo_thread->p24) /* packet 24 received */
                    {
                      int a, a1, e=39, l=3;
                      unsigned char *p = pageinfo_thread->p24;
                      do
                      {
                        for (;
                            l >= 2 && 0 == m_TXTCache.FlofPages[m_TXTCache.CurrentPage[magazine]][l];
                            l--)
                          ; /* find used linkindex */
                        for (;
                            e >= 1 && !isalnum(p[e]);
                            e--)
                          ; /* find end */
                        for (a = a1 = e - 1;
                            a >= 0 && p[a] >= ' ';
                            a--) /* find start */
                          if (p[a] > ' ')
                          a1 = a; /* first non-space */
                        if (a >= 0 && l >= 2)
                        {
                          strncpy(m_TXTCache.ADIPTable[m_TXTCache.FlofPages[m_TXTCache.CurrentPage[magazine]][l]], (const char*) &p[a1], 12);
                          if (e-a1 < 11)
                            m_TXTCache.ADIPTable[m_TXTCache.FlofPages[m_TXTCache.CurrentPage[magazine]][l]][e-a1+1] = '\0';
                        }
                        e = a - 1;
                        l--;
                      } while (l >= 2);
                    }
                  }
                }
                else if (descode == 4)  /* level 2.5 links (ignore level 3.5 links of /4 and /5) */
                {
                  int i;
                  Textp27_t *p;

                  if (!pageinfo_thread->ext)
                    pageinfo_thread->ext = (TextExtData_t*) calloc(1, sizeof(TextExtData_t));
                  if (!pageinfo_thread->ext)
                    continue;
                  if (!(pageinfo_thread->ext->p27))
                    pageinfo_thread->ext->p27 = (Textp27_t*) calloc(4, sizeof(Textp27_t));
                  if (!(pageinfo_thread->ext->p27))
                    continue;
                  p = pageinfo_thread->ext->p27;
                  for (i = 0; i < 4; i++)
                  {
                    int d1 = CDVDTeletextTools::deh24(&vtxt_row[6*i + 3]);
                    int d2 = CDVDTeletextTools::deh24(&vtxt_row[6*i + 6]);
                    if (d1 < 0 || d2 < 0)
                      continue;

                    p->local = i & 0x01;
                    p->drcs = !!(i & 0x02);
                    p->l25 = !!(d1 & 0x04);
                    p->l35 = !!(d1 & 0x08);
                    p->page =
                      (((d1 & 0x000003c0) >> 6) |
                       ((d1 & 0x0003c000) >> (14-4)) |
                       ((d1 & 0x00003800) >> (11-8))) ^
                      (dehamming[vtxt_row[0]] << 8);
                    if (p->page < 0x100)
                      p->page += 0x800;
                    p->subpage = d2 >> 2;
                    if ((p->page & 0xff) == 0xff)
                      p->page = 0;
                    else if (p->page > 0x899)
                    {
                      // workaround for crash on RTL Shop ...
                      // sorry.. i dont understand whats going wrong here :)
                      continue;
                    }
                    else if (m_TXTCache.astCachetable[p->page][0])  /* link valid && linked page cached */
                    {
                      TextPageinfo_t *pageinfo_link = &(m_TXTCache.astCachetable[p->page][0]->pageinfo);
                      if (p->local)
                        pageinfo_link->function = p->drcs ? FUNC_DRCS : FUNC_POP;
                      else
                        pageinfo_link->function = p->drcs ? FUNC_GDRCS : FUNC_GPOP;
                    }
                    p++; /*  */
                  }
                }
              }
              else if (packet_number == 26)
              {
                int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */
                if (descode == 0xff)
                  continue;

                if (!pageinfo_thread->ext)
                  pageinfo_thread->ext = (TextExtData_t*) calloc(1, sizeof(TextExtData_t));
                if (!pageinfo_thread->ext)
                  continue;
                if (!(pageinfo_thread->ext->p26[descode]))
                  pageinfo_thread->ext->p26[descode] = (unsigned char*) malloc(13 * 3);
                if (pageinfo_thread->ext->p26[descode])
                  memcpy(pageinfo_thread->ext->p26[descode], &vtxt_row[3], 13 * 3);
              }
              else if (packet_number == 28)
              {
                int descode = dehamming[vtxt_row[2]]; /* designation code (0..15) */

                if (descode == 0xff)
                  continue;

                if (descode != 2)
                {
                  int t1 = CDVDTeletextTools::deh24(&vtxt_row[7-4]);
                  pageinfo_thread->function = t1 & 0x0f;
                  if (!pageinfo_thread->nationalvalid)
                  {
                    pageinfo_thread->nationalvalid = 1;
                    pageinfo_thread->national = (t1>>4) & 0x07;
                  }
                }

                switch (descode) /* designation code */
                {
                  case 0: /* basic level 1 page */
                    Decode_p2829(vtxt_row, &(pageinfo_thread->ext));
                    break;
                  case 1: /* G0/G1 designation for older decoders, level 3.5: DCLUT4/16, colors for multicolored bitmaps */
                    break; /* ignore */
                  case 2: /* page key */
                    break; /* ignore */
                  case 3: /* types of PTUs in DRCS */
                    break; //! @todo implement
                  case 4: /* CLUTs 0/1, only level 3.5 */
                    break; /* ignore */
                  default:
                    break; /* invalid, ignore */
                } /* switch designation code */
              }
              else if (packet_number == 30)
              {
                m_TXTCache.line30 = "";
                for (int i=26-4; i <= 45-4; i++) /* station ID */
                  m_TXTCache.line30.append(1, deparity[vtxt_row[i]]);
              }
            }

            /* set update flag */
            if (m_TXTCache.CurrentPage[magazine] == m_TXTCache.Page && m_TXTCache.CurrentSubPage[magazine] != -1)
            {
              SavePage(m_TXTCache.CurrentPage[magazine], m_TXTCache.CurrentSubPage[magazine], pagedata[magazine]);
              m_TXTCache.PageUpdate = true;
//              doupdate = 0;
              if (!m_TXTCache.ZapSubpageManual)
                m_TXTCache.SubPage = m_TXTCache.CurrentSubPage[magazine];
            }
          }
        }
      }
    }
    else if (pMsg->IsType(CDVDMsg::PLAYER_SETSPEED))
    {
      m_speed = static_cast<CDVDMsgInt*>(pMsg)->m_value;
    }
    else if (pMsg->IsType(CDVDMsg::GENERAL_FLUSH)
          || pMsg->IsType(CDVDMsg::GENERAL_RESET))
    {
      ResetTeletextCache();
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
  if(!m_messageQueue.IsInited())
    return;
  /* flush using message as this get's called from VideoPlayer thread */
  /* and any demux packet that has been taken out of queue need to */
  /* be disposed of before we flush */
  m_messageQueue.Flush();
  m_messageQueue.Put(new CDVDMsg(CDVDMsg::GENERAL_FLUSH));
}

void CDVDTeletextData::Decode_p2829(unsigned char *vtxt_row, TextExtData_t **ptExtData)
{
  unsigned int bitsleft, colorindex;
  unsigned char *p;
  int t1 = CDVDTeletextTools::deh24(&vtxt_row[7-4]);
  int t2 = CDVDTeletextTools::deh24(&vtxt_row[10-4]);

  if (t1 < 0 || t2 < 0)
    return;

  if (!(*ptExtData))
    (*ptExtData) = (TextExtData_t*) calloc(1, sizeof(TextExtData_t));
  if (!(*ptExtData))
    return;

  (*ptExtData)->p28Received = 1;
  (*ptExtData)->DefaultCharset = (t1>>7) & 0x7f;
  (*ptExtData)->SecondCharset = ((t1>>14) & 0x0f) | ((t2<<4) & 0x70);
  (*ptExtData)->LSP = !!(t2 & 0x08);
  (*ptExtData)->RSP = !!(t2 & 0x10);
  (*ptExtData)->SPL25 = !!(t2 & 0x20);
  (*ptExtData)->LSPColumns = (t2>>6) & 0x0f;

  bitsleft = 8; /* # of bits not evaluated in val */
  t2 >>= 10; /* current data */
  p = &vtxt_row[13-4];  /* pointer to next data triplet */
  for (colorindex = 0; colorindex < 16; colorindex++)
  {
    if (bitsleft < 12)
    {
      t2 |= CDVDTeletextTools::deh24(p) << bitsleft;
      if (t2 < 0)  /* hamming error */
        break;
      p += 3;
      bitsleft += 18;
    }
    (*ptExtData)->bgr[colorindex] = t2 & 0x0fff;
    bitsleft -= 12;
    t2 >>= 12;
  }
  if (t2 < 0 || bitsleft != 14)
  {
    (*ptExtData)->p28Received = 0;
    return;
  }
  (*ptExtData)->DefScreenColor = t2 & 0x1f;
  t2 >>= 5;
  (*ptExtData)->DefRowColor = t2 & 0x1f;
  (*ptExtData)->BlackBgSubst = !!(t2 & 0x20);
  t2 >>= 6;
  (*ptExtData)->ColorTableRemapping = t2 & 0x07;
}

void CDVDTeletextData::SavePage(int p, int sp, unsigned char* buffer)
{
  CSingleLock lock(m_critSection);
  TextCachedPage_t* pg = m_TXTCache.astCachetable[p][sp];
  if (!pg)
  {
    CLog::Log(LOGERROR, "CDVDTeletextData: trying to save a not allocated page!!");
    return;
  }

  memcpy(pg->data, buffer, 23*40);
}

void CDVDTeletextData::LoadPage(int p, int sp, unsigned char* buffer)
{
  CSingleLock lock(m_critSection);
  TextCachedPage_t* pg = m_TXTCache.astCachetable[p][sp];
  if (!pg)
  {
    CLog::Log(LOGERROR, "CDVDTeletextData: trying to load a not allocated page!!");
    return;
  }

  memcpy(buffer, pg->data, 23*40);
}

void CDVDTeletextData::ErasePage(int magazine)
{
  CSingleLock lock(m_critSection);
  TextCachedPage_t* pg = m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]];
  if (pg)
  {
    memset(&(pg->pageinfo), 0, sizeof(TextPageinfo_t));  /* struct pageinfo */
    memset(pg->p0, ' ', 24);
    memset(pg->data, ' ', 23*40);
  }
}

void CDVDTeletextData::AllocateCache(int magazine)
{
  /* check cachetable and allocate memory if needed */
  if (m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]] == 0)
  {
    m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]] = new TextCachedPage_t;
    if (m_TXTCache.astCachetable[m_TXTCache.CurrentPage[magazine]][m_TXTCache.CurrentSubPage[magazine]] )
    {
      ErasePage(magazine);
      m_TXTCache.CachedPages++;
    }
  }
}

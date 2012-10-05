/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DVDOverlayCodecSSA.h"
#include "DVDOverlaySSA.h"
#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecs.h"
#include "DVDClock.h"
#include "Util.h"
#include "utils/AutoPtrHandle.h"

using namespace AUTOPTR;
using namespace std;

CDVDOverlayCodecSSA::CDVDOverlayCodecSSA() : CDVDOverlayCodec("SSA Subtitle Decoder")
{
  m_pOverlay = NULL;
  m_libass   = NULL;
  m_order    = 0;
}

CDVDOverlayCodecSSA::~CDVDOverlayCodecSSA()
{
  Dispose();
}

bool CDVDOverlayCodecSSA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if(hints.codec != CODEC_ID_SSA)
    return false;

  Dispose();

  m_hints  = hints;
  m_libass = new CDVDSubtitlesLibass();
  return m_libass->DecodeHeader((char *)hints.extradata, hints.extrasize);
}

void CDVDOverlayCodecSSA::Dispose()
{
  if(m_libass)
    SAFE_RELEASE(m_libass);

  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
}

int CDVDOverlayCodecSSA::Decode(DemuxPacket *pPacket)
{
  if(m_pOverlay)
    SAFE_RELEASE(m_pOverlay);
  
  if(!pPacket)
    return OC_ERROR;
  
  double pts = pPacket->dts != DVD_NOPTS_VALUE ? pPacket->dts : pPacket->pts;
  BYTE *data = pPacket->pData;
  int size = pPacket->iSize;
  double duration = pPacket->duration;

  if(strncmp((const char*)data, "Dialogue:", 9) == 0)
  {
    int    sh, sm, ss, sc, eh, em, es, ec;
    size_t pos;
    CStdString      line, line2;
    CStdStringArray lines;
    CUtil::Tokenize((const char*)data, lines, "\r\n");
    for(size_t i=0; i<lines.size(); i++)
    {
      line = lines[i];
      line.Trim();
      auto_aptr<char> layer(new char[line.length()+1]);

      if(sscanf(line.c_str(), "%*[^:]:%[^,],%d:%d:%d%*c%d,%d:%d:%d%*c%d"
                            , layer.get(), &sh, &sm, &ss, &sc, &eh,&em, &es, &ec) != 9)
        continue;

      duration  = (eh*360000.0)+(em*6000.0)+(es*100.0)+ec;
      if(pts == DVD_NOPTS_VALUE)
        pts = duration;
      duration -= (sh*360000.0)+(sm*6000.0)+(ss*100.0)+sc;
      duration *= 10000;

      pos = line.find_first_of(",", 0);
      pos = line.find_first_of(",", pos+1);
      pos = line.find_first_of(",", pos+1);
      if(pos == CStdString::npos)
        continue;

      line2.Format("%d,%s,%s", m_order++, layer.get(), line.Mid(pos+1));

      if(!m_libass->DecodeDemuxPkt((char*)line2.c_str(), line2.length(), pts, duration))
        continue;

      if(m_pOverlay == NULL)
      {
        m_pOverlay = new CDVDOverlaySSA(m_libass);
        m_pOverlay->iPTSStartTime = pts;
        m_pOverlay->iPTSStopTime  = pts + duration;
      }

      if(m_pOverlay->iPTSStopTime < pts + duration)
        m_pOverlay->iPTSStopTime  = pts + duration;
    }
  }
  else
  {
    if(m_libass->DecodeDemuxPkt((char*)data, size, pts, duration))
        m_pOverlay = new CDVDOverlaySSA(m_libass);
  }

  return OC_OVERLAY;
}
void CDVDOverlayCodecSSA::Reset()
{
  Dispose();
  m_order  = 0;
  m_libass = new CDVDSubtitlesLibass();
  m_libass->DecodeHeader((char *)m_hints.extradata, m_hints.extrasize);
}

void CDVDOverlayCodecSSA::Flush()
{
  Reset();
}

CDVDOverlay* CDVDOverlayCodecSSA::GetOverlay()
{
  if(m_pOverlay)
  {
    CDVDOverlay* overlay = m_pOverlay;
    m_pOverlay = NULL;
    return overlay;
  }
  return NULL;
}



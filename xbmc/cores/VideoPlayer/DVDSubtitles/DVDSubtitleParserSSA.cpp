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

#include "DVDSubtitleParserSSA.h"
#include "DVDCodecs/Overlay/DVDOverlaySSA.h"
#include "TimingConstants.h"
#include "utils/log.h"

CDVDSubtitleParserSSA::CDVDSubtitleParserSSA(std::unique_ptr<CDVDSubtitleStream> && pStream, const std::string& strFile)
    : CDVDSubtitleParserText(std::move(pStream), strFile)
{
  m_libass = new CDVDSubtitlesLibass();
}

CDVDSubtitleParserSSA::~CDVDSubtitleParserSSA()
{
  Dispose();
}

bool CDVDSubtitleParserSSA::Open(CDVDStreamInfo &hints)
{

  if (!CDVDSubtitleParserText::Open())
    return false;

  std::string buffer = m_pStream->m_stringstream.str();
  if(!m_libass->CreateTrack((char*) buffer.c_str(), buffer.length()))
    return false;

  //Creating the overlays by going through the list of ass_events
  ASS_Event* assEvent = m_libass->GetEvents();
  int numEvents = m_libass->GetNrOfEvents();

  for(int i=0; i < numEvents; i++)
  {
    ASS_Event* curEvent =  (assEvent+i);
    if (curEvent)
    {
      CDVDOverlaySSA* overlay = new CDVDOverlaySSA(m_libass);

      overlay->iPTSStartTime = (double)curEvent->Start * (DVD_TIME_BASE / 1000);
      overlay->iPTSStopTime  = (double)(curEvent->Start + curEvent->Duration) * (DVD_TIME_BASE / 1000);

      overlay->replace = true;
      m_collection.Add(overlay);
    }
  }
  m_collection.Sort();
  return true;
}

void CDVDSubtitleParserSSA::Dispose()
{
  if(m_libass)
  {
    SAFE_RELEASE(m_libass);
    CLog::Log(LOGINFO, "SSA Parser: Releasing reference to ASS Library");
  }
  CDVDSubtitleParserCollection::Dispose();
}

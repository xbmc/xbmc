/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#include "Edl.h"
#include "StringUtils.h"
#include "Util.h"
#include "FileSystem/File.h"
#include "FileSystem/CMythFile.h"
#include "AdvancedSettings.h"
#include "utils/log.h"

extern "C"
{
#include "lib/libcmyth/cmyth.h"
}

using namespace std;

#define MPLAYER_EDL_FILENAME "special://temp/xbmc.edl"
#define COMSKIP_HEADER "FILE PROCESSING COMPLETE"
#define VIDEOREDO_HEADER "<Version>2"
#define VIDEOREDO_TAG_CUT "<Cut>"
#define VIDEOREDO_TAG_SCENE "<SceneMarker "

using namespace XFILE;

CEdl::CEdl()
{
  Clear();
}

CEdl::~CEdl()
{
  Clear();
}

void CEdl::Clear()
{
  if (CFile::Exists(MPLAYER_EDL_FILENAME))
    CFile::Delete(MPLAYER_EDL_FILENAME);

  m_vecCuts.clear();
  m_vecSceneMarkers.clear();
  m_iTotalCutTime = 0;
}

bool CEdl::ReadFiles(const CStdString& strMovie, const float fFramesPerSecond)
{
  CLog::Log(LOGDEBUG, "%s - checking for any edit decision list (EDL) files for: %s", __FUNCTION__,
            strMovie.c_str());

  /*
   * Read any available format until a valid EDL related file is found.
   * 
   * TODO: Surely there's a better way to do this bFound shenanigans.
   */
  bool bFound = false;

  if (!bFound)
    bFound = ReadVideoReDo(strMovie);

  if (!bFound)
    bFound = ReadEdl(strMovie);

  if (!bFound)
    bFound = ReadComskip(strMovie, fFramesPerSecond);

  if (!bFound)
    bFound = ReadBeyondTV(strMovie);

  if (bFound)
  {
    MergeShortCommBreaks();
    WriteMPlayerEdl();
  }
  return bFound;
}

bool CEdl::ReadEdl(const CStdString& strMovie)
{
  Clear();

  CStdString edlFilename;
  CUtil::ReplaceExtension(strMovie, ".edl", edlFilename);
  if (!CFile::Exists(edlFilename))
    return false;

  CFile edlFile;
  if (!edlFile.Open(edlFilename))
  {
    CLog::Log(LOGERROR, "%s - Could not open EDL file: %s", __FUNCTION__, edlFilename.c_str());
    return false;
  }

  bool bValid = true;
  char szBuffer[1024];
  int iLine = 0;
  while (bValid && edlFile.ReadString(szBuffer, 1023))
  {
    iLine++;

    double dStart, dEnd;
    int iAction;
    if (sscanf(szBuffer, "%lf %lf %i", &dStart, &dEnd, &iAction) == 3)
    {
      if (dStart == dEnd) // Ignore zero length cuts in generated EDL files
        continue;

      Cut cut;
      cut.start = (int)dStart * 1000; // ms to s
      cut.end = (int)dEnd * 1000; // ms to s

      switch (iAction)
      {
      case 0:
        cut.action = CUT;
        bValid = AddCut(cut);
        break;
      case 1:
        cut.action = MUTE;
        bValid = AddCut(cut);
        break;
      case 2:
        bValid = AddSceneMarker(cut.end);
        break;
      case 3:
        cut.action = COMM_BREAK;
        bValid = AddCut(cut);
        break;
      default:
        bValid = false;
        continue;
      }
    }
    else
      bValid = false;
  }
  edlFile.Close();

  if (!bValid)
  {
    CLog::Log(LOGERROR, "%s - Invalid EDL file: %s. Error in line %i. Clearing any valid cuts or scenes found.",
              __FUNCTION__, edlFilename.c_str(), iLine);
    Clear();
    return false;
  }
  else if (HasCut() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "%s - Read %i cuts and %i scene markers in EDL file: %s", __FUNCTION__,
              m_vecCuts.size(), m_vecSceneMarkers.size(), edlFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No cuts or scene markers found in EDL file: %s", __FUNCTION__,
              edlFilename.c_str());
    return false;
  }
}

bool CEdl::ReadComskip(const CStdString& strMovie, const float fFramesPerSecond)
{
  Clear();

  CStdString comskipFilename;
  CUtil::ReplaceExtension(strMovie, ".txt", comskipFilename);
  if (!CFile::Exists(comskipFilename))
    return false;

  CFile comskipFile;
  if (!comskipFile.Open(comskipFilename))
  {
    CLog::Log(LOGERROR, "%s - Could not open Comskip file: %s", __FUNCTION__, comskipFilename.c_str());
    return false;
  }

  char szBuffer[1024];
  if (comskipFile.ReadString(szBuffer, 1023)
  &&  strncmp(szBuffer, COMSKIP_HEADER, strlen(COMSKIP_HEADER)) != 0) // Line 1.
  {
    CLog::Log(LOGERROR, "%s - Invalid Comskip file: %s. Error reading line 1 - expected '%s' at start.",
              __FUNCTION__, comskipFilename.c_str(), COMSKIP_HEADER);
    comskipFile.Close();
    return false;
  }

  int iFrames;
  float fFrameRate;
  if (sscanf(szBuffer, "FILE PROCESSING COMPLETE %i FRAMES AT %f", &iFrames, &fFrameRate) != 2)
  {
    /*
     * Not all generated Comskip files have the frame rate information.
     */
    fFrameRate = fFramesPerSecond;
    CLog::Log(LOGWARNING, "%s - Frame rate not in Comskip file. Using detected frames per second: %.3f",
              __FUNCTION__, fFrameRate);
  }
  else
    fFrameRate /= 100; // Reduce by factor of 100 to get fps.

  comskipFile.ReadString(szBuffer, 1023); // Line 2. Ignore "-------------"

  bool bValid = true;
  int iLine = 2;
  while (bValid && comskipFile.ReadString(szBuffer, 1023)) // Line 3 and onwards.
  {
    iLine++;
    double dStartFrame, dEndFrame;
    if (sscanf(szBuffer, "%lf %lf", &dStartFrame, &dEndFrame) == 2)
    {
      Cut cut;
      cut.start = (__int64)(dStartFrame / fFrameRate * 1000);
      cut.end = (__int64)(dEndFrame / fFrameRate * 1000);
      cut.action = COMM_BREAK;
      bValid = AddCut(cut);
    }
    else
      bValid = false;
  }
  comskipFile.Close();

  if (!bValid)
  {
    CLog::Log(LOGERROR, "%s - Invalid Comskip file: %s. Error on line %i. Clearing any valid commercial breaks found.",
              __FUNCTION__, comskipFilename.c_str(), iLine);
    Clear();
    return false;
  }
  else if (HasCut())
  {
    CLog::Log(LOGDEBUG, "%s - Read %i commercial breaks from Comskip file: %s", __FUNCTION__, m_vecCuts.size(),
              comskipFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No commercial breaks found in Comskip file: %s", __FUNCTION__, comskipFilename.c_str());
    return false;
  }
}

bool CEdl::ReadVideoReDo(const CStdString& strMovie)
{
  /*
   * VideoReDo file is strange. Tags are XML like, but it isn't an XML file.
   *
   * http://www.videoredo.com/
   */

  Clear();
  CStdString videoReDoFilename;
  CUtil::ReplaceExtension(strMovie, ".Vprj", videoReDoFilename);
  if (!CFile::Exists(videoReDoFilename))
    return false;

  CFile videoReDoFile;
  if (!videoReDoFile.Open(videoReDoFilename))
  {
    CLog::Log(LOGERROR, "%s - Could not open VideoReDo file: %s", __FUNCTION__, videoReDoFilename.c_str());
    return false;
  }

  char szBuffer[1024];
  if (videoReDoFile.ReadString(szBuffer, 1023)
  &&  strncmp(szBuffer, VIDEOREDO_HEADER, strlen(VIDEOREDO_HEADER)) != 0)
  {
    CLog::Log(LOGERROR, "%s - Invalid VideoReDo file: %s. Error reading line 1 - expected %s. Only version 2 files are supported.",
              __FUNCTION__, videoReDoFilename.c_str(), VIDEOREDO_HEADER);
    videoReDoFile.Close();
    return false;
  }

  int iLine = 1;
  bool bValid = true;
  while (bValid && videoReDoFile.ReadString(szBuffer, 1023))
  {
    iLine++;
    if (strncmp(szBuffer, VIDEOREDO_TAG_CUT, strlen(VIDEOREDO_TAG_CUT)) == 0) // Found the <Cut> tag
    {
      /*
       * double is used as 32 bit float would overflow.
       */
      double dStart, dEnd;
      if (sscanf(szBuffer + strlen(VIDEOREDO_TAG_CUT), "%lf:%lf", &dStart, &dEnd) == 2)
      {
        /*
         *  Times need adjusting by 1/10,000 to get ms.
         */
        Cut cut;
        cut.start = (__int64)(dStart / 10000);
        cut.end = (__int64)(dEnd / 10000);
        cut.action = CUT;
        bValid = AddCut(cut);
      }
      else
        bValid = false;
    }
    else if (strncmp(szBuffer, VIDEOREDO_TAG_SCENE, strlen(VIDEOREDO_TAG_SCENE)) == 0) // Found the <SceneMarker > tag
    {
      int iScene;
      double dSceneMarker;
      if (sscanf(szBuffer + strlen(VIDEOREDO_TAG_SCENE), " %i>%lf", &iScene, &dSceneMarker) == 2)
        bValid = AddSceneMarker((int)dSceneMarker / 10000); // Times need adjusting by 1/10,000 to get ms.
      else
        bValid = false;
    }
    /*
     * Ignore any other tags.
     */
  }
  videoReDoFile.Close();

  if (!bValid)
  {
    CLog::Log(LOGERROR, "%s - Invalid VideoReDo file: %s. Error in line %i. Clearing any valid cuts or scenes found.",
              __FUNCTION__, videoReDoFilename.c_str(), iLine);
    Clear();
    return false;
  }
  else if (HasCut() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "%s - Read %i cuts and %i scene markers in VideoReDo file: %s", __FUNCTION__,
              m_vecCuts.size(), m_vecSceneMarkers.size(), videoReDoFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No cuts or scene markers found in VideoReDo file: %s", __FUNCTION__,
              videoReDoFilename.c_str());
    return false;
  }
}

bool CEdl::ReadBeyondTV(const CStdString& strMovie)
{
  Clear();

  CStdString beyondTVFilename = strMovie + ".chapters.xml";
  if (!CFile::Exists(beyondTVFilename))
    return false;

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(beyondTVFilename))
  {
    CLog::Log(LOGERROR, "%s - Could not load Beyond TV file: %s. %s", __FUNCTION__, beyondTVFilename.c_str(),
              xmlDoc.ErrorDesc());
    return false;
  }

  if (xmlDoc.Error())
  {
    CLog::Log(LOGERROR, "%s - Could not parse Beyond TV file: %s. %s", __FUNCTION__, beyondTVFilename.c_str(),
              xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRoot = xmlDoc.RootElement();
  if (!pRoot || strcmp(pRoot->Value(), "cutlist"))
  {
    CLog::Log(LOGERROR, "%s - Invalid Beyond TV file: %s. Expected root node to be <cutlist>", __FUNCTION__,
              beyondTVFilename.c_str());
    return false;
  }

  bool bValid = true;
  TiXmlElement *pRegion = pRoot->FirstChildElement("Region");
  while (bValid && pRegion)
  {
    TiXmlElement *pStart = pRegion->FirstChildElement("start");
    TiXmlElement *pEnd = pRegion->FirstChildElement("end");
    if (pStart && pEnd && pStart->FirstChild() && pEnd->FirstChild())
    {
      /*
       * Need to divide the start and end times by a factor of 10,000 to get msec.
       * E.g. <start comment="00:02:44.9980867">1649980867</start>
       *
       * Use atof so doesn't overflow 32 bit float or integer / long.
       * E.g. <end comment="0:26:49.0000009">16090090000</end>
       *
       * Don't use atoll even though it is more correct as it isn't natively supported by
       * Visual Studio.
       *
       * GetText() returns 0 if there were any problems and will subsequently be rejected in AddCut().
       */
      Cut cut;
      cut.start = (__int64)(atof(pStart->GetText()) / 10000);
      cut.end = (__int64)(atof(pEnd->GetText()) / 10000);
      cut.action = COMM_BREAK;
      bValid = AddCut(cut);
    }
    else
      bValid = false;

    pRegion = pRegion->NextSiblingElement("Region");
  }
  if (!bValid)
  {
    CLog::Log(LOGERROR, "%s - Invalid Beyond TV file: %s. Clearing any valid commercial breaks found.", __FUNCTION__,
              beyondTVFilename.c_str());
    Clear();
    return false;
  }
  else if (HasCut())
  {
    CLog::Log(LOGDEBUG, "%s - Read %i commercial breaks from Beyond TV file: %s", __FUNCTION__, m_vecCuts.size(),
              beyondTVFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No commercial breaks found in Beyond TV file: %s", __FUNCTION__,
              beyondTVFilename.c_str());
    return false;
  }
}

bool CEdl::AddCut(const Cut& cut)
{
  if (cut.action != CUT && cut.action != MUTE && cut.action != COMM_BREAK)
  {
    CLog::Log(LOGERROR, "%s - Not a CUT, MUTE, or COMM_BREAK! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (cut.start < 0)
  {
    CLog::Log(LOGERROR, "%s - Before start! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (cut.start >= cut.end)
  {
    CLog::Log(LOGERROR, "%s - Times are around the wrong way or the same! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (InCut(cut.start) || InCut(cut.end))
  {
    CLog::Log(LOGERROR, "%s - Start or end is in an existing cut! [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (cut.start < m_vecCuts[i].start && cut.end > m_vecCuts[i].end)
    {
      CLog::Log(LOGERROR, "%s - Cut surrounds an existing cut! [%s - %s], %d", __FUNCTION__,
                MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
                cut.action);
      return false;
    }
  }

  /*
   * Insert cut in the list in the right position (ALL algorithms assume cuts are in ascending order)
   */
  if (m_vecCuts.empty() || cut.start > m_vecCuts.back().start)
  {
    CLog::Log(LOGDEBUG, "%s - Pushing new cut to back [%s - %s], %d", __FUNCTION__,
              MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    m_vecCuts.push_back(cut);
  }
  else
  {
    vector<Cut>::iterator pCurrentCut;
    for (pCurrentCut = m_vecCuts.begin(); pCurrentCut != m_vecCuts.end(); pCurrentCut++)
    {
      if (cut.start < pCurrentCut->start)
      {
        CLog::Log(LOGDEBUG, "%s - Inserting new cut [%s - %s], %d", __FUNCTION__,
                  MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
                  cut.action);
        m_vecCuts.insert(pCurrentCut, cut);
        break;
      }
    }
  }

  if (cut.action == CUT)
    m_iTotalCutTime += cut.end - cut.start;

  return true;
}

bool CEdl::AddSceneMarker(const __int64 iSceneMarker)
{
  Cut cut;

  if (InCut(iSceneMarker, &cut) && cut.action == CUT) // Only works for current cuts.
    return false;

  CLog::Log(LOGDEBUG, "%s - Inserting new scene marker: %s", __FUNCTION__,
            MillisecondsToTimeString(iSceneMarker).c_str());
  m_vecSceneMarkers.push_back(iSceneMarker); // Unsorted

  return true;
}

bool CEdl::WriteMPlayerEdl()
{
  if (!HasCut())
    return false;

  CFile mplayerEdlFile;
  if (!mplayerEdlFile.OpenForWrite(MPLAYER_EDL_FILENAME, true))
  {
    CLog::Log(LOGERROR, "%s - Error opening MPlayer EDL file for writing: %s", __FUNCTION__,
              MPLAYER_EDL_FILENAME);
    return false;
  }

  CStdString strBuffer;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    /* 
     * MPlayer doesn't understand the scene marker (2) or commercial break (3) identifiers that XBMC
     * supports in EDL files.
     *
     * http://www.mplayerhq.hu/DOCS/HTML/en/edl.html
     *
     * Write out mutes (1) directly. Treat commercial breaks as cuts (everything other than MUTES = 0).
     */
    strBuffer.AppendFormat("%.3f\t%.3f\t%i\n", (float)(m_vecCuts[i].start / 1000),
                                               (float)(m_vecCuts[i].end / 1000),
                                               m_vecCuts[i].action == MUTE ? 1 : 0);
  }
  mplayerEdlFile.Write(strBuffer.c_str(), strBuffer.size());
  mplayerEdlFile.Close();

  CLog::Log(LOGDEBUG, "%s - MPlayer EDL file written to: %s", __FUNCTION__, MPLAYER_EDL_FILENAME);

  return true;
}

bool CEdl::HasCut()
{
  return !m_vecCuts.empty();
}

__int64 CEdl::GetTotalCutTime()
{
  return m_iTotalCutTime; // ms
}

__int64 CEdl::RemoveCutTime(__int64 iSeek)
{
  if (!HasCut())
    return iSeek;

  __int64 iCutTime = 0;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == CUT)
    {
      if (iSeek >= m_vecCuts[i].start && iSeek <= m_vecCuts[i].end) // Inside cut
        iCutTime += iSeek - m_vecCuts[i].start - 1; // Decrease cut length by 1ms to jump over end boundary.
      else if (iSeek >= m_vecCuts[i].start) // Cut has already been passed over.
        iCutTime += m_vecCuts[i].end - m_vecCuts[i].start;
    }
  }
  return iSeek - iCutTime;
}

__int64 CEdl::RestoreCutTime(__int64 iClock)
{
  if (!HasCut())
    return iClock;

  __int64 iSeek = iClock;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == CUT && iSeek >= m_vecCuts[i].start)
      iSeek += m_vecCuts[i].end - m_vecCuts[i].start;
  }

  return iSeek;
}

bool CEdl::HasSceneMarker()
{
  return !m_vecSceneMarkers.empty();
}

CStdString CEdl::GetInfo()
{
  /*
   * TODO: Update wiki (http://xbmc.org/wiki/?title=EDL_(commercial_skipping)_and_SceneMarker_support)
   * if these different status strings are used.
   */
  CStdString strInfo = "";
  if (HasCut())
  {
    int cutCount = 0, muteCount = 0, commBreakCount = 0;
    for (int i = 0; i < (int)m_vecCuts.size(); i++)
    {
      switch (m_vecCuts[i].action)
      {
      case CUT:
        cutCount++;
        break;
      case MUTE:
        muteCount++;
        break;
      case COMM_BREAK:
        commBreakCount++;
        break;
      }
    }
    if (cutCount > 0)
      strInfo.AppendFormat("c%i", cutCount);
    if (muteCount > 0)
      strInfo.AppendFormat("m%i", muteCount);
    if (commBreakCount > 0)
      strInfo.AppendFormat("b%i", commBreakCount);
  }
  if (HasSceneMarker())
    strInfo.AppendFormat("s%i", m_vecSceneMarkers.size());

  return strInfo.IsEmpty() ? "-" : strInfo;
}

bool CEdl::InCut(const __int64 iSeek, Cut *pCut)
{
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (iSeek < m_vecCuts[i].start) // Early exit if not even up to the cut start time.
      return false;

    if (iSeek >= m_vecCuts[i].start && iSeek <= m_vecCuts[i].end) // Inside cut.
    {
      if (pCut)
        *pCut = m_vecCuts[i];
      return true;
    }
  }

  return false;
}

bool CEdl::GetNextSceneMarker(bool bPlus, const __int64 iClock, __int64 *iSceneMarker)
{
  if (!HasSceneMarker())
    return false;

  __int64 iSeek = RestoreCutTime(iClock);

  __int64 iDiff = 10 * 60 * 60 * 1000; // 10 hours to ms.
  bool bFound = false;

  if (bPlus) // Find closest scene forwards
  {
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] > iSeek) && ((m_vecSceneMarkers[i] - iSeek) < iDiff))
      {
        iDiff = m_vecSceneMarkers[i] - iSeek;
        *iSceneMarker = m_vecSceneMarkers[i];
        bFound = true;
      }
    }
  }
  else // Find closest scene backwards
  {
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] < iSeek) && ((iSeek - m_vecSceneMarkers[i]) < iDiff))
      {
        iDiff = iSeek - m_vecSceneMarkers[i];
        *iSceneMarker = m_vecSceneMarkers[i];
        bFound = true;
      }
    }
  }

  /*
   * If the scene marker is in a cut then return the end of the cut. Can't guarantee that this is
   * picked up when scene markers are added.
   */
  Cut cut;
  if (bFound && InCut(*iSceneMarker, &cut) && cut.action == CUT)
    *iSceneMarker = cut.end;

  return bFound;
}

CStdString CEdl::MillisecondsToTimeString(const __int64 iMilliseconds)
{
  CStdString strTimeString = "";
  StringUtils::SecondsToTimeString((long)(iMilliseconds / 1000), strTimeString, TIME_FORMAT_HH_MM_SS); // milliseconds to seconds
  strTimeString.AppendFormat(".%03i", iMilliseconds % 1000);
  return strTimeString;
}

bool CEdl::ReadMythCommBreaks(const CURL& url, const float fFramesPerSecond)
{
  Clear();

  /*
   * Exists() sets up all the internal bits needed for GetCommBreakList().
   */
  CCMythFile mythFile;
  if (!mythFile.Exists(url))
    return false;

  CLog::Log(LOGDEBUG, "%s - Reading commercial breaks from MythTV for: %s", __FUNCTION__,
            url.GetFileName().c_str());

  cmyth_commbreaklist_t commbreaklist;
  if (!mythFile.GetCommBreakList(commbreaklist))
  {
    CLog::Log(LOGERROR, "%s - Error getting commercial breaks from MythTV for: %s", __FUNCTION__,
              url.GetFileName().c_str());
    return false;
  }

  for (int i = 0; i < (int)commbreaklist->commbreak_count; i++)
  {
    cmyth_commbreak_t commbreak = commbreaklist->commbreak_list[i];

    Cut cut;
    cut.action = COMM_BREAK;
    cut.start = (int)(commbreak->start_mark / fFramesPerSecond * 1000);
    cut.end = (int)(commbreak->end_mark / fFramesPerSecond * 1000);

    /*
     * Detection isn't perfect near the edges so autowind by a small amount into each end of the
     * detected commercial break.
     *
     * TODO: Advanced setting for the autowind amount. Perhaps one for fowards and one for backwards?
     */
    int autowind = 2 * 1000; // 2 seconds in ms
    if (cut.start > 0) // Only autowind forwards if not at the start.
      cut.start += autowind;
    if (cut.end > cut.start + autowind) // Only autowind if it won't go back past the start (should never happen).
      cut.end -= autowind;

    if (!AddCut(cut)) // Log and continue with errors while still testing.
      CLog::Log(LOGERROR, "%s - Invalid commercial break [%s - %s] found in MythTV for: %s. autowind: %d. Continuing anyway.",
                __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(),
                MillisecondsToTimeString(cut.end).c_str(), url.GetFileName().c_str(), autowind);
  }

  if (HasCut())
  {
    CLog::Log(LOGDEBUG, "%s - Added %i commercial breaks from MythTV for: %s. Used detected frame rate of %.3f fps to calculate times from the frame markers.",
              __FUNCTION__, m_vecCuts.size(), url.GetFileName().c_str(), fFramesPerSecond);
    MergeShortCommBreaks();
    /*
     * No point writing the MPlayer EDL file as it won't be able to play the MythTV files anyway.
     */
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No commercial breaks found in MythTV for: %s", __FUNCTION__,
              url.GetFileName().c_str());
    return false;
  }
}

void CEdl::MergeShortCommBreaks()
{
  if (g_advancedSettings.m_bEdlMergeShortCommBreaks)
  {
    for (int i = 0; i < (int)m_vecCuts.size() - 1; i++)
    {
      if ((m_vecCuts[i].action == COMM_BREAK && m_vecCuts[i + 1].action == COMM_BREAK)
      &&  (m_vecCuts[i + 1].end - m_vecCuts[i].start < g_advancedSettings.m_iEdlMaxCommBreakLength * 1000) // s to ms
      &&  (m_vecCuts[i + 1].start - m_vecCuts[i].end < g_advancedSettings.m_iEdlMaxCommBreakGap * 1000)) // s to ms
      {
        Cut commBreak;
        commBreak.action = COMM_BREAK;
        commBreak.start = m_vecCuts[i].start;
        commBreak.end = m_vecCuts[i + 1].end;

        CLog::Log(LOGDEBUG, "%s - Consolidating commercial break [%s - %s] and [%s - %s] to: [%s - %s]", __FUNCTION__,
                  MillisecondsToTimeString(m_vecCuts[i].start).c_str(), MillisecondsToTimeString(m_vecCuts[i].end).c_str(),
                  MillisecondsToTimeString(m_vecCuts[i + 1].start).c_str(), MillisecondsToTimeString(m_vecCuts[i + 1].end).c_str(),
                  MillisecondsToTimeString(commBreak.start).c_str(), MillisecondsToTimeString(commBreak.end).c_str());

        /*
         * Erase old cuts and insert the new merged one.
         */
        m_vecCuts.erase(m_vecCuts.begin() + i, m_vecCuts.begin() + i + 2);
        m_vecCuts.insert(m_vecCuts.begin() + i, commBreak);

        i--; // Reduce i to see if the next break is also within the max commercial break length.
      }
    }

    /*
     * Remove any commercial breaks shorter than the minimum (unless at the start)
     */
    for (int i = 0; i < (int)m_vecCuts.size(); i++)
    {
      if (m_vecCuts[i].action == COMM_BREAK
      &&  m_vecCuts[i].start > 0
      && (m_vecCuts[i].end - m_vecCuts[i].start) < g_advancedSettings.m_iEdlMinCommBreakLength * 1000)
      {
        CLog::Log(LOGDEBUG, "%s - Removing short commercial break [%s - %s]. Minimum length: %i seconds", __FUNCTION__,
                  MillisecondsToTimeString(m_vecCuts[i].start).c_str(), MillisecondsToTimeString(m_vecCuts[i].end).c_str(),
                  g_advancedSettings.m_iEdlMinCommBreakLength);
        m_vecCuts.erase(m_vecCuts.begin() + i);

        i--;
      }
    }
  }

  /*
   * Add in scene markers at the start and end of the commercial breaks.
   */
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == COMM_BREAK)
    {
      if (m_vecCuts[i].start > 0) // Don't add a scene marker at the start.
        AddSceneMarker(m_vecCuts[i].start);
      AddSceneMarker(m_vecCuts[i].end);
    }
  }
  return;
}

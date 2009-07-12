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
#include "include.h"
#include "Util.h"
#include "FileSystem/File.h"

using namespace std;

#define CACHED_EDL_FILENAME "special://temp/xbmc.edl"
#define COMSKIPSTR "FILE PROCESSING COMPLETE"
#define VRSTR "<Version>2"
#define VRCUT "<Cut>"
#define VRSCENE "<SceneMarker "
#define BTVSTR "<cutlist>"
#define BTVCUT "<Region><start"
#define BTVSTREND "</cutlist>"

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
  if (CFile::Exists(CACHED_EDL_FILENAME))
    CFile::Delete(CACHED_EDL_FILENAME);

  m_vecCuts.clear();
  m_vecSceneMarkers.clear();
  m_iTotalCutTime = 0;
}

bool CEdl::ReadFiles(const CStdString& strMovie)
{
  // Try to read any available format until a valid edl is read
  Clear();

  ReadVideoRedo(strMovie);
  if (!HasCut() && !HasSceneMarker())
    ReadEdl(strMovie);

  if (!HasCut() && !HasSceneMarker())
    ReadComskip(strMovie);

  if (!HasCut() && !HasSceneMarker())
    ReadBeyondTV(strMovie);

  if (HasCut() || HasSceneMarker())
    WriteMPlayerEdl();

  return HasCut() || HasSceneMarker();
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
    CLog::Log(LOGDEBUG, "%s - Read %i cuts and %i scene markers in EDL file: %s", __FUNCTION__, m_vecCuts.size(),
              m_vecSceneMarkers.size(), edlFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No cuts or scene markers found in EDL file: %s", __FUNCTION__, edlFilename.c_str());
    return false;
  }
}

bool CEdl::ReadComskip(const CStdString& strMovie)
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
  if (comskipFile.ReadString(szBuffer, 1023) && (strncmp(szBuffer, COMSKIPSTR, strlen(COMSKIPSTR)) != 0)) // Line 1.
  {
    CLog::Log(LOGERROR, "%s - Invalid Comskip file: %s. Error reading line 1 - expected '%s' at start.",
              __FUNCTION__, comskipFilename.c_str(), COMSKIPSTR);
    comskipFile.Close();
    return false;
  }
  
  int iFrames;
  int iFrameRate;
  if (sscanf(szBuffer, "FILE PROCESSING COMPLETE %i FRAMES AT %i", &iFrames, &iFrameRate) != 2)
  {
    /*
     * Not all generated Comskip files have the frame rate information.
     */
    CLog::Log(LOGERROR, "%s - Frame rate not found on line 1 in Comskip file. Only version 2 files and upwards are supported.",
              __FUNCTION__);
    comskipFile.Close();
    return false;
  }

  float fFrameRate = iFrameRate / 100; // Reduce by factor of 100 to get fps.

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
      cut.action = CUT;
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

bool CEdl::ReadVideoRedo(const CStdString& strMovie)
{
  Clear();
  CStdString videoRedoFilename;
  CUtil::ReplaceExtension(strMovie, ".VPrj", videoRedoFilename);
  if (!CFile::Exists(videoRedoFilename))
    return false;

  CFile videoRedoFile;
  if (!videoRedoFile.Open(videoRedoFilename))
  {
    CLog::Log(LOGERROR, "%s - Could not open VideoRedo file: %s", __FUNCTION__, videoRedoFilename.c_str());
    return false;
  }

  char szBuffer[1024];
  if (videoRedoFile.ReadString(szBuffer, 1023) && strncmp(szBuffer, VRSTR, strlen(VRSTR)) != 0)
  {
    CLog::Log(LOGERROR, "%s - Invalid VideoRedo file: %s. Error reading line 1 - expected %s. Only version 2 files are supported.",
              __FUNCTION__, videoRedoFilename.c_str(), VRSTR);
    videoRedoFile.Close();
    return false;
  }

  int iLine = 1;
  bool bValid = true;
  while (bValid && videoRedoFile.ReadString(szBuffer, 1023))
  {
    iLine++;
    if (strncmp(szBuffer, VRCUT, strlen(VRCUT)) == 0)
    {
      double dStartFrame;
      double dEndFrame;
      if (sscanf(szBuffer + strlen(VRCUT), "%lf:%lf", &dStartFrame, &dEndFrame) == 2)
      {
        Cut cut;
        cut.start = (__int64)(dStartFrame / 10000);
        cut.end = (__int64)(dEndFrame / 10000);
        cut.action = CUT;
        bValid = AddCut(cut);
      }
      else
        bValid = false;
    }
    else if (strncmp(szBuffer, VRSCENE, strlen(VRSCENE)) == 0)
    {
      int iScene;
      double dSceneMarker;
      if (sscanf(szBuffer + strlen(VRSCENE), " %i>%lf", &iScene, &dSceneMarker) == 2)
        bValid = AddSceneMarker(dSceneMarker / 10000);
      else
        bValid = false;
    }
    // Ignore any other tags.
  }
  videoRedoFile.Close();

  if (!bValid)
  {
    CLog::Log(LOGERROR, "%s - Invalid VideoRedo file: %s. Error in line %i. Clearing any valid cuts or scenes found.",
              __FUNCTION__, videoRedoFilename.c_str(), iLine);
    Clear();
    return false;
  }
  else if (HasCut() || HasSceneMarker())
  {
    CLog::Log(LOGDEBUG, "%s - Read %i cuts and %i scene markers in VideoRedo file: %s", __FUNCTION__,
              m_vecCuts.size(), m_vecSceneMarkers.size(), videoRedoFilename.c_str());
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - No cuts or scene markers found in VideoRedo file: %s", __FUNCTION__,
              videoRedoFilename.c_str());
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
    CLog::Log(LOGERROR, "Unable to parse chapters.xml file: %s", xmlDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *root = xmlDoc.RootElement();
  if (!root || strcmp(root->Value(), "cutlist"))
  {
    CLog::Log(LOGERROR, "Unable to parse chapters.xml file: %s", xmlDoc.ErrorDesc());
    return false;
  }

  bool bValid = true;
  TiXmlElement *pRegion = root->FirstChildElement("Region");
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
      cut.action = CUT;
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
  if (cut.action != CUT && cut.action != MUTE)
  {
    CLog::Log(LOGERROR, "%s - Not a CUT or MUTE! [%s - %s], %d",
              __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (cut.start < 0)
  {
    CLog::Log(LOGERROR, "%s - Before start! [%s - %s], %d", __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(),
              MillisecondsToTimeString(cut.end).c_str(), cut.action);
    return false;
  }
  
  if (cut.start >= cut.end)
  {
    CLog::Log(LOGERROR, "%s - Times are around the wrong way or the same! [%s - %s], %d",
              __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  if (InCut(cut.start) || InCut(cut.end))
  {
    CLog::Log(LOGERROR, "%s - Start or end is in an existing cut! [%s - %s], %d",
              __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
              cut.action);
    return false;
  }

  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (cut.start < m_vecCuts[i].start && cut.end > m_vecCuts[i].end)
    {
      CLog::Log(LOGERROR, "%s - Cut surrounds an existing cut! [%s - %s], %d",
                __FUNCTION__, MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
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
    vector<Cut>::iterator vitr;
    for (vitr = m_vecCuts.begin(); vitr != m_vecCuts.end(); vitr++)
    {
      if (cut.start < vitr->start)
      {
        CLog::Log(LOGDEBUG, "%s - Inserting new cut [%s - %s], %d", __FUNCTION__,
                  MillisecondsToTimeString(cut.start).c_str(), MillisecondsToTimeString(cut.end).c_str(),
                  cut.action);
        m_vecCuts.insert(vitr, cut);
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

  if (InCut(iSceneMarker, &cut) && cut.action == CUT)// this only works for current cutpoints, no for cutpoints added later.
    return false;

  CLog::Log(LOGDEBUG, "%s - Inserting new scene marker: %s", __FUNCTION__, MillisecondsToTimeString(iSceneMarker).c_str());
  m_vecSceneMarkers.push_back(iSceneMarker); // Unsorted

  return true;
}

bool CEdl::WriteMPlayerEdl()
{
  if (!HasCut())
    return false;

  CFile mplayerEdlFile;
  if (!mplayerEdlFile.OpenForWrite(CACHED_EDL_FILENAME, true))
  {
    CLog::Log(LOGERROR, "%s - Error opening MPlayer EDL file for writing: %s", __FUNCTION__, CACHED_EDL_FILENAME);
    return false;
  }

  CStdString write;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    write.AppendFormat("%.2f\t%.2f\t%i\n", ((double)m_vecCuts[i].start) / 1000, ((double)m_vecCuts[i].end) / 1000,
                       m_vecCuts[i].action);
  }
  mplayerEdlFile.Write(write.c_str(), write.size());
  mplayerEdlFile.Close();

  CLog::Log(LOGDEBUG, "%s - MPlayer EDL file written to: %s", __FUNCTION__, CACHED_EDL_FILENAME);

  return true;
}

bool CEdl::HasCut()
{
  return !m_vecCuts.empty();
}

__int64 CEdl::GetTotalCutTime()
{
  return m_iTotalCutTime; //msec.
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

char CEdl::GetEdlStatus()
{
  char cEdlStatus = 'n';

  if (HasCut() && HasSceneMarker())
    cEdlStatus = 'b';
  else if (HasCut())
    cEdlStatus = 'e';
  else if (HasSceneMarker())
    cEdlStatus = 's';

  return cEdlStatus;
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
  if (!HasCut())
    return false;

  // Need absolute time.
  __int64 iSeek = RestoreCutTime(iClock);
  __int64 iDiff = 10 * 60 * 60 * 1000; // 10 hours to ms.
  bool bFound = false;

  if (bPlus)
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
  else
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

CStdString CEdl::MillisecondsToTimeString(const int iMilliseconds)
{
  CStdString strTimeString = "";
  StringUtils::SecondsToTimeString(iMilliseconds / 1000, strTimeString, TIME_FORMAT_HH_MM_SS); // milliseconds to seconds
  strTimeString.AppendFormat(".%03i", iMilliseconds % 1000);
  return strTimeString;
}


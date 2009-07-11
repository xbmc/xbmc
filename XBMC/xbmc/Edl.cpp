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
#include "Application.h"

using namespace std;

#ifndef WSAEVENT  //Needed for XBMC_PC somehow.
#define WSAEVENT                HANDLE
#endif

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

  iFrameRate /= 100; // Reduce by factor of 100 to get fps.

  comskipFile.ReadString(szBuffer, 1023); // Line 2. Ignore "-------------"

  bool bValid = true;
  while (bValid && comskipFile.ReadString(szBuffer, 1023))
  {
    double dStartFrame;
    double dEndFrame;
    if (sscanf(szBuffer, "%lf %lf", &dStartFrame, &dEndFrame) == 2)
    {
      Cut cut;
      cut.start = (__int64)(dStartFrame / iFrameRate * 1000);
      cut.end = (__int64)(dEndFrame / iFrameRate * 1000);
      cut.action = CUT;
      bValid = AddCut(cut);
    }
    else
      bValid = false;
  }
  comskipFile.Close();

  if (bValid && HasCut())
  {
    CLog::Log(LOGDEBUG, "CEdl: Read ComSkip.");
  }
  else
    Clear();

  return bValid;
}

bool CEdl::ReadVideoRedo(const CStdString& strMovie)
{
  Clear();
  CStdString videoRedoFilename;
  CUtil::ReplaceExtension(strMovie, ".VPrj", videoRedoFilename);

  bool bValid = false;
  CFile videoRedoFile;
  if (CFile::Exists(videoRedoFilename) && videoRedoFile.Open(videoRedoFilename))
  {
    bValid = true;
    char szBuffer[1024];
    if (videoRedoFile.ReadString(szBuffer, 1023) && (strncmp(szBuffer, VRSTR, strlen(VRSTR)) == 0))
    {
      videoRedoFile.ReadString(szBuffer, 1023); // read away Filename
      while (bValid && videoRedoFile.ReadString(szBuffer, 1023))
      {
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
        }
        else
        {
          if (strncmp(szBuffer, VRSCENE, strlen(VRSCENE)) == 0)
          {
            int iScene;
            double dSceneMarker;
            if (sscanf(szBuffer + strlen(VRSCENE), " %i>%lf", &iScene, &dSceneMarker) == 2)
              bValid = AddSceneMarker(dSceneMarker / 10000);
            else
              bValid = false;
          }
        }
        // Ignore other tags.
      }
    }
    videoRedoFile.Close();
  }

  if (bValid && (HasCut() || HasSceneMarker()))
    CLog::Log(LOGDEBUG, "CEdl: Read VidoRedo.");
  else
    Clear();
  return bValid;
}

bool CEdl::ReadBeyondTV(const CStdString& strMovie)
{
  Clear();
  CStdString beyondTVFilename = strMovie + ".chapters.xml";

  if (!CFile::Exists(beyondTVFilename))
    return false;

  CFileStream file;
  if (!file.Open(beyondTVFilename))
  {
    CLog::Log(LOGDEBUG, "%s failed to read file %s", __FUNCTION__, beyondTVFilename.c_str());
    return false;
  }

  TiXmlDocument xmlDoc;
  file >> xmlDoc;

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
  TiXmlElement *pRegion = root->FirstChildElement("Region");
  while (pRegion)
  {
    TiXmlElement *pStart = pRegion->FirstChildElement("start");
    TiXmlElement *pEnd = pRegion->FirstChildElement("end");
    if (pStart && pEnd && pStart->FirstChild() && pEnd->FirstChild())
    {
      double dStartFrame = atof(pStart->FirstChild()->Value());
      double dEndFrame = atof(pEnd->FirstChild()->Value());
      Cut cut;
      cut.start = (__int64)(dStartFrame / 10000);
      cut.end = (__int64)(dEndFrame / 10000);
      cut.action = CUT;
      AddCut(cut); // just ignore it if it fails
    }
    pRegion = pRegion->NextSiblingElement("Region");
  }

  if (HasCut())
  {
    CLog::Log(LOGDEBUG, "CEdl: Read BeyondTV.");
    return true;
  }
  CLog::Log(LOGDEBUG, "CEdl: Failed to get cutpoints in BeyondTV file.");
  return false;
}

bool CEdl::AddCut(const Cut& cut)
{
  vector<Cut>::iterator vitr;

  if (cut.start >= cut.end)
    return false;

  if (cut.start < 0)
    return false;

  if (InCut(cut.start) || InCut(cut.end))
    return false;

  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (cut.start < m_vecCuts[i].start && cut.end > m_vecCuts[i].end)
      return false;
  }

  // Insert cutpoint in list.
  if (m_vecCuts.empty() || m_vecCuts.back().start < cut.start)
    m_vecCuts.push_back(cut);
  else
  {
    for (vitr = m_vecCuts.begin(); vitr != m_vecCuts.end(); ++vitr)
    {
      if (vitr->start > cut.start)
      {
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
  m_vecSceneMarkers.push_back(iSceneMarker); // Unsorted

  return true;
}

bool CEdl::WriteMPlayerEdl()
{
  CFile mplayerEdlFile;
  if (mplayerEdlFile.OpenForWrite(CACHED_EDL_FILENAME, true))
  {
    CStdString write;
    for (int i = 0; i < (int)m_vecCuts.size(); i++)
    {
      if ((m_vecCuts[i].action == CUT) || (m_vecCuts[i].action == MUTE))
      {
        write.AppendFormat("%.2f\t%.2f\t%i\n", ((double)m_vecCuts[i].start) / 1000, ((double)m_vecCuts[i].end) / 1000,
                           m_vecCuts[i].action);
      }
    }
    mplayerEdlFile.Write(write.c_str(), write.size());
    mplayerEdlFile.Close();
    CLog::Log(LOGDEBUG, "CEdl: EDL Cached.");
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "CEdl: Error writing EDL to cache.");
    Clear();
    return false;
  }
}

bool CEdl::HasCut()
{
  return !m_vecCuts.empty();
}

__int64 CEdl::GetTotalCutTime()
{
  if (!HasCut())
    return 0;
  else
    return m_iTotalCutTime; //msec.
}

__int64 CEdl::RemoveCutTime(__int64 iTime)
{
  __int64 iCutTime = 0;

  if (!HasCut())
    return iTime;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == CUT && m_vecCuts[i].end <= iTime)
      iCutTime += m_vecCuts[i].end - m_vecCuts[i].start;
  }

  return iTime - iCutTime;
}

__int64 CEdl::RestoreCutTime(__int64 iTime)
{
  if (!HasCut())
    return iTime;
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].action == CUT && m_vecCuts[i].start <= iTime)
      iTime += m_vecCuts[i].end - m_vecCuts[i].start;
  }

  return iTime;
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

bool CEdl::InCut(__int64 iAbsSeek, Cut *pCurCut)
{
  for (int i = 0; i < (int)m_vecCuts.size(); i++)
  {
    if (m_vecCuts[i].start <= iAbsSeek && m_vecCuts[i].end >= iAbsSeek)
    {
      if (pCurCut)
        *pCurCut = m_vecCuts[i];
      return true;
    }
    else
    {
      if (m_vecCuts[i].start > iAbsSeek)
        return false;
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
  __int64 iNextScene = -1;
  __int64 iDiff;
  Cut cut;

  if (bPlus)
  {
    iDiff = (__int64 )99999999999999LL;
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] > iSeek) && ((m_vecSceneMarkers[i] - iSeek) < iDiff))
      {
        iDiff = m_vecSceneMarkers[i] - iSeek;
        iNextScene = m_vecSceneMarkers[i];
      }
    }
  }
  else
  {
    iSeek = (iSeek > 5000) ? iSeek - 5000 : 0; // Jump over nearby scene to avoid getting stuck.
    iDiff = (__int64 )99999999999999LL;
    for (int i = 0; i < (int)m_vecSceneMarkers.size(); i++)
    {
      if ((m_vecSceneMarkers[i] < iSeek) && ((iSeek - m_vecSceneMarkers[i]) < iDiff))
      {
        iDiff = iSeek - m_vecSceneMarkers[i];
        iNextScene = m_vecSceneMarkers[i];
      }
    }
  }

  // extra check for incutpoint, we cannot filter this out reliable earlier.
  if (InCut(iNextScene, &cut) && cut.action == CUT)
    return false;

  *iSceneMarker = iNextScene;

  return (iNextScene != -1);
}

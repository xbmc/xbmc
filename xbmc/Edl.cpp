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
  Reset();
}

CEdl::~CEdl()
{
  Reset();
}

void CEdl::Reset()
{
  if (CFile::Exists(CACHED_EDL_FILENAME))
    CFile::Delete(CACHED_EDL_FILENAME);

  m_vecCuts.clear();
  m_vecScenelist.clear();
  m_iTotalCutTime=0;
}


bool CEdl::ReadnCacheAny(const CStdString& strMovie)
{
  // Try to read any available format until a valid edl is read
  Reset();

  ReadVideoRedo(strMovie);
  if (!HasCut() && !HasSceneMarker())
    ReadEdl(strMovie);

  if (!HasCut() && !HasSceneMarker())
    ReadComskip(strMovie);

  if (!HasCut() && !HasSceneMarker())
    ReadBeyondTV(strMovie);

  if (HasCut() || HasSceneMarker())
    CacheEdl();

  return HasCut() || HasSceneMarker();
}

bool CEdl::ReadEdl(const CStdString& strMovie)
{
  Cut tmpCut;
  CFile CutFile;
  double dCutStart, dCutEnd;
  bool tmpValid=false;

  Reset();
  CStdString edlFilename;
  CUtil::ReplaceExtension(strMovie, ".edl", edlFilename);
  if ( CFile::Exists(edlFilename) && CutFile.Open(edlFilename) )
  {
    tmpValid=true;
    char szBuffer[1024];
    while (tmpValid && CutFile.ReadString(szBuffer, 1023))
    {
      if( sscanf( szBuffer, "%lf %lf %i", &dCutStart, &dCutEnd, (int*) &tmpCut.action ) == 3)
      {
        tmpCut.start=(__int64)(dCutStart*1000);
        tmpCut.end=(__int64)(dCutEnd*1000);
        if ( tmpCut.action==CUT || tmpCut.action==MUTE )
          tmpValid=AddCutpoint(tmpCut);
        else if ( tmpCut.action==SCENE )
          tmpValid=AddScene(tmpCut);
        else
          tmpValid=false;
      }
      else
        tmpValid=false;
    }
    CutFile.Close();
  }
  if (tmpValid && (HasCut() || HasSceneMarker()))
    CLog::Log(LOGDEBUG, "CEdl: Read Edl.");
  else
    Reset();

  return tmpValid;
}

bool CEdl::ReadComskip(const CStdString& strMovie)
{
  Cut tmpCut;
  CFile CutFile;
  int iFramerate;
  int iFrames;
  double dStartframe;
  double dEndframe;
  bool tmpValid=false;

  Reset();
  CStdString comskipFilename;
  CUtil::ReplaceExtension(strMovie, ".txt", comskipFilename);

  if ( CFile::Exists(comskipFilename) && CutFile.Open(comskipFilename) )
  {
    tmpValid=true;
    char szBuffer[1024];
    if (CutFile.ReadString(szBuffer, 1023) && (strncmp(szBuffer,COMSKIPSTR, strlen(COMSKIPSTR))==0))
    {
      if (sscanf(szBuffer, "FILE PROCESSING COMPLETE %i FRAMES AT %i", &iFrames, &iFramerate) == 2)
      {
        iFramerate=iFramerate/100;
        CutFile.ReadString(szBuffer, 1023); // read away -------------
        while (tmpValid && CutFile.ReadString(szBuffer, 1023))
        {
          if (sscanf(szBuffer, "%lf %lf", &dStartframe, &dEndframe) == 2)
          {
            tmpCut.start=(__int64)(dStartframe/iFramerate*1000);
            tmpCut.end=(__int64)(dEndframe/iFramerate*1000);
            tmpCut.action=CUT;
            tmpValid=AddCutpoint(tmpCut);
          }
          else
            tmpValid=false;
        }
      }
      else
        tmpValid=false;
    }
    CutFile.Close();
  }
  if (tmpValid && HasCut())
  {
    CLog::Log(LOGDEBUG, "CEdl: Read ComSkip.");
  }
  else
    Reset();

  return tmpValid;
}

bool CEdl::ReadVideoRedo(const CStdString& strMovie)
{
  Cut tmpCut;
  CFile CutFile;
  int iScene;
  double dStartframe;
  double dEndframe;
  bool tmpValid=false;

  Reset();
  CStdString videoRedoFilename;
  CUtil::ReplaceExtension(strMovie, ".VPrj", videoRedoFilename);

  if (CFile::Exists(videoRedoFilename) && CutFile.Open(videoRedoFilename))
  {
    tmpValid=true;
    char szBuffer[1024];
    if (CutFile.ReadString(szBuffer, 1023) && (strncmp(szBuffer,VRSTR,strlen(VRSTR))==0))
    {
      CutFile.ReadString(szBuffer, 1023); // read away Filename
      while (tmpValid && CutFile.ReadString(szBuffer, 1023))
      {
        if(strncmp(szBuffer,VRCUT,strlen(VRCUT))==0)
        {
          if (sscanf( szBuffer+strlen(VRCUT), "%lf:%lf", &dStartframe, &dEndframe ) == 2)
          {
            tmpCut.start=(__int64)(dStartframe/10000);
            tmpCut.end=(__int64)(dEndframe/10000);
            tmpCut.action=CUT;
            tmpValid=AddCutpoint(tmpCut);
          }
        }
        else
        {
          if (strncmp(szBuffer,VRSCENE,strlen(VRSCENE))==0)
          {
            if (sscanf(szBuffer+strlen(VRSCENE), " %i>%lf",&iScene, &dStartframe)==2)
              tmpValid=AddScene(tmpCut);
            else
              tmpValid=false;
          }
        }
        // Ignore other tags.
      }
    }
    CutFile.Close();
  }

  if (tmpValid && (HasCut() || HasSceneMarker()))
    CLog::Log(LOGDEBUG, "CEdl: Read VidoRedo.");
  else
    Reset();
  return tmpValid;
}

bool CEdl::ReadBeyondTV(const CStdString& strMovie)
{
  Reset();
  CStdString beyondTVFilename=strMovie+".chapters.xml";

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
  TiXmlElement *Region = root->FirstChildElement("Region");
  while (Region)
  {
    TiXmlElement *Start = Region->FirstChildElement("start");
    TiXmlElement *End = Region->FirstChildElement("end");
    if ( Start && End && Start->FirstChild() && End->FirstChild() )
    {
      double dStartframe=atof(Start->FirstChild()->Value());
      double dEndframe=atof(End->FirstChild()->Value());
      Cut cut;
      cut.start=(__int64)(dStartframe/10000);
      cut.end=(__int64)(dEndframe/10000);
      cut.action=CUT;
      AddCutpoint(cut); // just ignore it if it fails
    }
    Region = Region->NextSiblingElement("Region");
  }

  if (HasCut())
  {
    CLog::Log(LOGDEBUG, "CEdl: Read BeyondTV.");
    return true;
  }
  CLog::Log(LOGDEBUG, "CEdl: Failed to get cutpoints in BeyondTV file.");
  return false;
}



bool CEdl::AddCutpoint(const Cut& NewCut)
{
  vector<Cut>::iterator vitr;

  if (NewCut.start >= NewCut.end)
    return false;

  if (NewCut.start < 0)
    return false;

  if (InCutpoint(NewCut.start) || InCutpoint(NewCut.end))
      return false;

  // Always returns 0 at this point?
  if (g_application.m_pPlayer->GetTotalTime() > 0
        && (NewCut.end > (g_application.m_pPlayer->GetTotalTime())*1000))

  for(int i = 0; i < (int)m_vecCuts.size(); i++ )
  {
    if ( NewCut.start < m_vecCuts[i].start && NewCut.end > m_vecCuts[i].end)
      return false;
  }

  // Insert cutpoint in list.
  if (m_vecCuts.empty() || m_vecCuts.back().start < NewCut.start)
    m_vecCuts.push_back(NewCut);
  else
  {
    for (vitr=m_vecCuts.begin(); vitr != m_vecCuts.end(); ++vitr)
    {
      if (vitr->start > NewCut.start)
      {
        m_vecCuts.insert(vitr, NewCut);
        break;
      }
    }
  }
  if (NewCut.action == CUT)
    m_iTotalCutTime+=NewCut.end-NewCut.start;

  return true;
}

bool CEdl::AddScene(const Cut& NewCut)
{
  Cut TmpCut;


  if (InCutpoint(NewCut.end, &TmpCut) && TmpCut.action == CUT)// this only works for current cutpoints, no for cutpoints added later.
      return false;
  m_vecScenelist.push_back(NewCut.end); // Unsorted

  return true;
}

bool CEdl::CacheEdl()
{
  CFile cacheFile;
  if (cacheFile.OpenForWrite(CACHED_EDL_FILENAME, true))
  {
    CStdString write;
    for(int i = 0; i < (int)m_vecCuts.size(); i++ )
    {
      if ((m_vecCuts[i].action==CUT) || (m_vecCuts[i].action==MUTE))
      {
        write.AppendFormat("%.2f\t%.2f\t%i\n",((double)m_vecCuts[i].start)/1000, ((double)m_vecCuts[i].end)/1000, m_vecCuts[i].action);
      }
    }
    cacheFile.Write(write.c_str(), write.size());
    cacheFile.Close();
    CLog::Log(LOGDEBUG, "CEdl: EDL Cached.");
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "CEdl: Error writing EDL to cache.");
    Reset();
    return false;
  }
}

bool CEdl::HasCut()
{
  return m_vecCuts.size() > 0;
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
  __int64 iCutTime=0;

  if (!HasCut())
    return iTime;
  for(int i = 0; i < (int)m_vecCuts.size(); i++ )
  {
    if (m_vecCuts[i].action==CUT && m_vecCuts[i].end <= iTime)
      iCutTime += m_vecCuts[i].end - m_vecCuts[i].start;
  }

  return iTime-iCutTime;
}

__int64 CEdl::RestoreCutTime(__int64 iTime)
{
  if (!HasCut())
    return iTime;
  for(int i = 0; i < (int)m_vecCuts.size(); i++ )
  {
    if (m_vecCuts[i].action==CUT && m_vecCuts[i].start <= iTime)
      iTime += m_vecCuts[i].end - m_vecCuts[i].start;
  }

  return iTime;
}

bool CEdl::HasSceneMarker()
{
  return m_vecScenelist.size() > 0;
}

char CEdl::GetEdlStatus()
{
  char cEdlStatus='n';

  if (HasCut() && HasSceneMarker())
    cEdlStatus='b';
  else if (HasCut())
    cEdlStatus='e';
  else if (HasSceneMarker())
    cEdlStatus='s';

  return cEdlStatus;
}

bool CEdl::InCutpoint(__int64 iAbsSeek, Cut *pCurCut)
{
  for(int i = 0; i < (int)m_vecCuts.size(); i++ )
  {
    if (m_vecCuts[i].start <= iAbsSeek && m_vecCuts[i].end >= iAbsSeek)
    {
      if (pCurCut)
        *pCurCut=m_vecCuts[i];
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

bool CEdl::SeekScene(bool bPlus, __int64 *iScenemarker)
{
  if (!HasCut())
    return false;

  // Need absolute time.
  __int64 iCurSeek=RestoreCutTime(g_application.m_pPlayer->GetTime());
  __int64 iNextScene=-1;
  __int64 iDiff;
  Cut TmpCut;

  if (bPlus)
  {
    iDiff=(__int64)99999999999999LL;
    for(int i = 0; i < (int)m_vecScenelist.size(); i++ )
    {
      if ( (m_vecScenelist[i] > iCurSeek) && ((m_vecScenelist[i]-iCurSeek) < iDiff))
      {
        iDiff=m_vecScenelist[i]-iCurSeek;
        iNextScene=m_vecScenelist[i];
      }
    }
  }
  else
  {
    iCurSeek = (iCurSeek>5000) ? iCurSeek-5000 : 0; // Jump over nearby scene to avoid getting stuck.
    iDiff=(__int64)99999999999999LL;
    for(int i = 0; i < (int)m_vecScenelist.size(); i++ )
    {
      if ((m_vecScenelist[i] < iCurSeek) && ((iCurSeek-m_vecScenelist[i]) < iDiff))
      {
        iDiff=iCurSeek-m_vecScenelist[i];
        iNextScene=m_vecScenelist[i];
      }
    }
  }

  // extra check for incutpoint, we cannot filter this out reliable earlier.
  if (InCutpoint(iNextScene, &TmpCut) && TmpCut.action == CUT)
    return false;

  // Make sure scene is in movie.
  if (iNextScene >= g_application.m_pPlayer->GetTotalTime()*1000+GetTotalCutTime())
    return false;

  *iScenemarker=iNextScene;

  return (iNextScene != -1);
}

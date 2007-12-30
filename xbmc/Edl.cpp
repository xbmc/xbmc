#include "Edl.h"
#include "include.h"
#include "stdafx.h"
#include "util.h"

#ifndef WSAEVENT  //Needed for XBMC_PC somehow.
#define WSAEVENT                HANDLE
#endif

#include "application.h"
#include "videodatabase.h"

using namespace XFILE;

CEdl::CEdl()
{
  Reset();
}

CEdl::~CEdl()
{
  if (IsCached())
  {
    CFile::Delete(CACHED_EDL_FILENAME);
  }
  Reset();
}

bool CEdl::ReadnCacheAny(const CStdString& strMovie)
{
  // Try to read any available format until a valid edl is read
  Reset();
  SetMovie(strMovie);

  ReadVideoRedo();
  if (!HaveCutpoints() && !HaveScenes())
  {
    ReadEdl();
  } 
  if (!HaveCutpoints() && !HaveScenes())
  {
    ReadComskip();
  } 
  if (!HaveCutpoints() && !HaveScenes())
  {
    ReadBeyondTV();
  }
  if (HaveCutpoints() || HaveScenes())
  {
    CacheEdl();
  }
  return IsCached();
}

bool CEdl::ReadEdl()
{
  Cut tmpCut;
  CFile CutFile;
  bool tmpValid=false;
      
  Reset();
  CUtil::ReplaceExtension(m_strMovie, ".edl", m_strEdlFilename);
  if ( CFile::Exists(m_strEdlFilename) && CutFile.Open(m_strEdlFilename) )
  {
    tmpValid=true;
    while (CutFile.ReadString(m_szBuffer, 1023) && tmpValid) 
    {
      if( sscanf( m_szBuffer, "%lf %lf %i", &tmpCut.CutStart, &tmpCut.CutEnd, &tmpCut.CutAction ) == 3)
      {
        if ( tmpCut.CutAction==CUT || tmpCut.CutAction==MUTE )
        {  
          tmpValid=AddCutpoint(tmpCut);
        }
        else if ( tmpCut.CutAction==SCENE )
        {
          tmpValid=AddScene(tmpCut);
        }
        else
        {
          tmpValid=false;
        }
      }
      else
      {
        tmpValid=false; 
      }
    }
    CutFile.Close();
  }
  if (tmpValid && (HaveCutpoints() || HaveScenes()))
  {
    CLog::Log(LOGDEBUG, "CEdl: Read Edl.");
  }
  else
  {
    Reset();
  }
  return tmpValid;
}


bool CEdl::ReadComskip()
{
  Cut tmpCut;
  CFile CutFile;
  int iFramerate;
  int iFrames;
  double dStartframe;
  double dEndframe;
  bool tmpValid=false;

  Reset();
  CUtil::ReplaceExtension(m_strMovie, ".txt", m_strEdlFilename);

  if ( CFile::Exists(m_strEdlFilename) && CutFile.Open(m_strEdlFilename) )
  {
    tmpValid=true;
    if (CutFile.ReadString(m_szBuffer, 1023) && (strncmp(m_szBuffer,COMSKIPSTR, strlen(COMSKIPSTR))==0))
    {
      if (sscanf(m_szBuffer, "FILE PROCESSING COMPLETE %i FRAMES AT %i", &iFrames, &iFramerate) == 2)
      {
        iFramerate=iFramerate/100;
        CutFile.ReadString(m_szBuffer, 1023); // read away -------------
        while (CutFile.ReadString(m_szBuffer, 1023) && tmpValid)
        {
          if (sscanf(m_szBuffer, "%lf %lf", &dStartframe, &dEndframe) == 2)
          {
            tmpCut.CutStart=dStartframe/iFramerate; 
            tmpCut.CutEnd=dEndframe/iFramerate;
            tmpCut.CutAction=CUT;
            tmpValid=AddCutpoint(tmpCut);
          }
          else
          {
            tmpValid=false; 
          }
        }
      }
      else
      {
        tmpValid=false;
      }
    }
    CutFile.Close();
  }
  if (tmpValid && HaveCutpoints())
  {
    CLog::Log(LOGDEBUG, "CEdl: Read ComSkip.");
    m_bCutpoints=true;
  }
  else
  {
    Reset();
  }
  return tmpValid;
}

bool CEdl::ReadVideoRedo()
{
  Cut tmpCut;
  CFile CutFile;
  int iScene;
  double dStartframe;
  double dEndframe;
  bool tmpValid=false;

  Reset();
  CUtil::ReplaceExtension(m_strMovie, ".VPrj", m_strEdlFilename);

  if (CFile::Exists(m_strEdlFilename) && CutFile.Open(m_strEdlFilename))
  {
    tmpValid=true;
    if (CutFile.ReadString(m_szBuffer, 1023) && (strncmp(m_szBuffer,VRSTR,strlen(VRSTR))==0))
    {
      CutFile.ReadString(m_szBuffer, 1023); // read away Filename
      while (CutFile.ReadString(m_szBuffer, 1023) && tmpValid)
      {
        if(strncmp(m_szBuffer,VRCUT,strlen(VRCUT))==0)
        {
          if (sscanf( m_szBuffer+strlen(VRCUT), "%lf:%lf", &dStartframe, &dEndframe ) == 2)
          {
            tmpCut.CutStart=dStartframe/10000000; 
            tmpCut.CutEnd=dEndframe/10000000;
            tmpCut.CutAction=CUT;
            tmpValid=AddCutpoint(tmpCut);
          }
        }
        else
        {
          if (strncmp(m_szBuffer,VRSCENE,strlen(VRSCENE))==0) 
          {
            if (sscanf(m_szBuffer+strlen(VRSCENE), " %i>%lf",&iScene, &dStartframe)==2)
            {
              tmpValid=AddScene(tmpCut);
            }
            else
            {
              tmpValid=false;
            }
          }
        }
        // Ignore other tags.
      }
    }
    CutFile.Close();
  }

  if (tmpValid && (HaveCutpoints() || HaveScenes()))
  {
    CLog::Log(LOGDEBUG, "CEdl: Read VidoRedo.");
  }
  else
  {
    Reset();
  }
  return tmpValid;
}

bool CEdl::ReadBeyondTV()
{
  Reset();
  m_strEdlFilename=m_strMovie+".chapters.xml";

  if (!CFile::Exists(m_strEdlFilename))
    return false;

  CFileStream file;
  if (!file.Open(m_strEdlFilename))
  {
    CLog::Log(LOGDEBUG, "%s failed to read file %s", __FUNCTION__, m_strEdlFilename.c_str());
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
      cut.CutStart=dStartframe/10000000; 
      cut.CutEnd=dEndframe/10000000;
      cut.CutAction=CUT;
      AddCutpoint(cut); // just ignore it if it fails
    }
    Region = Region->NextSiblingElement("Region");
  }

  if (HaveCutpoints())
  {
    CLog::Log(LOGDEBUG, "CEdl: Read BeyondTV.");
    m_bCutpoints=true;
    return true;
  }
  CLog::Log(LOGDEBUG, "CEdl: Failed to get cutpoints in BeyondTV file.");
  return false;
}

/*
bool CEdl::ReadVDR()
{
  Cut tmpCut;
  CFile CutFile;
  int iHours,iMinutes,iSeconds,iFrame;
  double dTimeStamp;
  bool tmpValid=false;
  bool CutStart=false;
  CStdString strEdlDirectory;

  Reset();
  CUtil::GetDirectory(m_strMovie, strEdlDirectory);
  m_strEdlFilename=strEdlDirectory + "marks.vdr";

  if (CFile::Exists(m_strEdlFilename))
  {
    if (CutFile.Open(m_strEdlFilename))
    {
      tmpValid=true;
      tmpCut.CutStart=0;
      tmpCut.CutAction=CUT;
      while (CutFile.ReadString(m_szBuffer, 1023) && tmpValid)
      {
        if( sscanf( m_szBuffer, "%i:%i:%i.%i", &iHours, &iMinutes, &iSeconds, &iFrame) )
        {
          dTimeStamp=iHours*3600+iMinutes*60+iSeconds+(double)iFrame/24.5; //Don't know framerate, guess something between PAL and NTSC
          if (!CutStart)
          {
            if (dTimeStamp!=0)
            {
              tmpCut.CutEnd=dTimeStamp;
              tmpValid=AddCutpoint(tmpCut);
            }
            else
            {
              // first timestamp 0? drop.
              CutStart=true;
            }
          }
          else
          {
            tmpCut.CutStart=dTimeStamp;
          }
        }
        else
        {
          CLog::Log(LOGDEBUG, "CEdl: Read VDR.");
          tmpValid=false; 
        }
      }
      if ( !CutStart && m_vecCutlist.size()>0 )
      {
        // CutEnd is end of movie
        tmpCut.CutEnd=(double)(g_application.m_pPlayer->GetTotalTime());
        tmpValid=AddCutpoint(tmpCut);
      }
      CutFile.Close();
    }
  }
  if (tmpValid)
  {
    m_bCutpoints=true;
  }
  else
  {
    Reset();
  }
  return tmpValid;
}
*/

void CEdl::Reset()
{
  m_vecCutlist.clear();
  m_vecScenelist.clear();
  m_bCutpoints=false;
  m_bCached=false;
  m_bScenes=false;
  m_strCachedEdl="";
  m_strEdlFilename="";
}

bool CEdl::AddCutpoint(const Cut& NewCut)
{
  bool bSuccess=false;
  if ((NewCut.CutEnd >= NewCut.CutStart)) // && !InCutpoint(NewCut.CutStart) && !InCutpoint(NewCut.CutEnd))
  {
    m_vecCutlist.push_back(NewCut);
    m_bCutpoints=true;
    bSuccess=true;
  }
  return bSuccess;
}

bool CEdl::AddScene(const Cut& NewCut)
{
  m_vecScenelist.push_back(NewCut.CutEnd);
  m_bScenes=true;
  return true; //No failures defined yet.
}
 

void CEdl::SetMovie(const CStdString& strMovie)
{
  m_strMovie=strMovie;
}

bool CEdl::CacheEdl()
{
  FILE* pFile;
  m_strCachedEdl=CACHED_EDL_FILENAME;
  m_bCached=false;
  pFile = fopen(m_strCachedEdl.c_str(), "w" );
  if ( pFile )
  {
    for(int i = 0; i < (int)m_vecCutlist.size(); i++ )
    {
      if ((m_vecCutlist[i].CutAction==CUT) || (m_vecCutlist[i].CutAction==MUTE))
      {
        fprintf(pFile,"%.2f\t%.2f\t%i\n",m_vecCutlist[i].CutStart, m_vecCutlist[i].CutEnd, m_vecCutlist[i].CutAction); 
      }
    }
    fclose(pFile);
    m_bCached=true;
    CLog::Log(LOGDEBUG, "CEdl: EDL Cached.");
  }
  else
  {
    CLog::Log(LOGERROR, "CEdl: Error writing EDL to cache.");
    Reset();
  }
  return m_bCached;
}

CStdString CEdl::GetCachedEdl()
{
  return m_strCachedEdl;
}

bool CEdl::HaveCutpoints()
{
  return m_bCutpoints;
}

bool CEdl::HaveScenes()
{
  return m_bScenes;
}

char CEdl::GetEdlStatus()
{
  char cEdlStatus='n';

  if (HaveCutpoints() && HaveScenes())
    cEdlStatus='b';
  else if (HaveCutpoints())
    cEdlStatus='e';
  else if (HaveScenes())
    cEdlStatus='s';

  return cEdlStatus;
}

  
bool CEdl::IsCached()
{
  return m_bCached;
}

bool CEdl::InCutpoint(double dAbsSeek, Cut *pCurCut)
{
  bool bInCut=false;

  for(int i = 0; i < (int)m_vecCutlist.size(); i++ )
  {
    if ((m_vecCutlist[i].CutAction == CUT) && (m_vecCutlist[i].CutStart < dAbsSeek) && (m_vecCutlist[i].CutEnd > dAbsSeek))
    {
      bInCut=true;
      *pCurCut=m_vecCutlist[i];
      i=(int)m_vecCutlist.size();
    }
  }
  return bInCut;
}

void CEdl::CompensateSeek(bool bPlus, int *iSeek)
{
  double dAbsSeek;
  double dCutLen;
  double dCurtime;
  Cut CurCut;
  
  if (HaveCutpoints())
  {
    dCurtime = (double)(g_application.m_pPlayer->GetTime())/1000;
    dAbsSeek = dCurtime + (double)*iSeek; 
    dAbsSeek = (dAbsSeek<0) ? 0 : dAbsSeek;  
    while (InCutpoint(dAbsSeek, &CurCut))
    {
      dCutLen = CurCut.CutEnd-CurCut.CutStart;
      dAbsSeek = bPlus ?  dAbsSeek+dCutLen : dAbsSeek-dCutLen;
      dAbsSeek = (dAbsSeek<0) ? 0 : dAbsSeek;
    }
    *iSeek=(int)(dAbsSeek-dCurtime); //Convert back to seconds
  }
}


bool CEdl::SeekScene(bool bPlus, __int64 *iScenemarker)
{
  double dCurSeek=(double)(g_application.m_pPlayer->GetTime())/1000;
  double dNextScene=-1;
  double dDiff;

  // Should we skip the scenemarker if inside a cutpoint? For now , we don't.
  if (bPlus)
  {
    dDiff=(double)99999999999999;
    for(int i = 0; i < (int)m_vecScenelist.size(); i++ )
    {
      if ( (m_vecScenelist[i] > dCurSeek) && ((m_vecScenelist[i]-dCurSeek) < dDiff))
      {
        dDiff=m_vecScenelist[i]-dCurSeek;
        dNextScene=m_vecScenelist[i];
      }
    }
  }
  else
  {
    dCurSeek = (dCurSeek>5) ? dCurSeek-5 : 0; // Jump over nearby scene to avoid getting stuck.
    dDiff=(double)99999999999999;
    for(int i = 0; i < (int)m_vecScenelist.size(); i++ )
    {
      if ((m_vecScenelist[i] < dCurSeek) && ((dCurSeek-m_vecScenelist[i]) < dDiff))
      {
        dDiff=dCurSeek-m_vecScenelist[i];
        dNextScene=m_vecScenelist[i];
      }
    }
  }
  *iScenemarker=(__int64)(dNextScene*1000);
   
  return (dNextScene != -1);
}

// Not used, untested. Maybe for future use.
void CEdl::AddBookmark(double dSceneMarker)
{
  CVideoDatabase dbs;
  if (dbs.Open())
  {
      CBookmark bookmark;
      if( g_application.m_pPlayer )
        bookmark.playerState = g_application.m_pPlayer->GetPlayerState();
      bookmark.player = CPlayerCoreFactory::GetPlayerName(g_application.GetCurrentPlayer());
      bookmark.timeInSeconds = dSceneMarker;
      bookmark.thumbNailImage.Empty();

      dbs.AddBookMarkToFile(g_application.CurrentFile(),bookmark, CBookmark::STANDARD);
  }
}

#include "pch.h"
#include "ChaptersManager.h"

CChaptersManager *CChaptersManager::m_pSingleton = NULL;

CChaptersManager::CChaptersManager(void):
 m_currentChapter(-1),
 m_init(false)
{
}

CChaptersManager::~CChaptersManager(void)
{
  for (std::map<long, SChapterInfos *>::iterator it = m_chapters.begin();
    it != m_chapters.end(); ++it)
    delete it->second;

  CLog::Log(LOGDEBUG, "%s Ressources released", __FUNCTION__);

  //SAFE_RELEASE(m_pIAMExtendedSeeking);
}

CChaptersManager *CChaptersManager::getSingleton()
{
  return (m_pSingleton) ? m_pSingleton : (m_pSingleton = new CChaptersManager());
}

void CChaptersManager::Destroy()
{
  delete m_pSingleton;
  m_pSingleton = NULL;
}

int CChaptersManager::GetChapterCount()
{
  return m_chapters.size();
}

int CChaptersManager::GetChapter()
{
  if ( m_chapters.empty())
    return -1;
  else
    return m_currentChapter;
}

void CChaptersManager::GetChapterName(CStdString& strChapterName)
{
  if (m_currentChapter == -1)
    return;

  strChapterName = m_chapters[m_currentChapter]->name;
}

void CChaptersManager::UpdateChapters()
{
  if (m_pIAMExtendedSeeking && !m_chapters.empty()
    && GetChapterCount() > 1)
    m_pIAMExtendedSeeking->get_CurrentMarker(&m_currentChapter);
}

bool CChaptersManager::LoadChapters()
{
  if (! CFGLoader::Filters.Splitter.pBF)
    return false;

  CStdString splitterName = CFGLoader::Filters.Splitter.osdname;

  CLog::Log(LOGDEBUG, "%s Looking for chapters in \"%s\"", __FUNCTION__, splitterName.c_str());

  if (SUCCEEDED(CFGLoader::Filters.Splitter.pBF->QueryInterface(IID_IAMExtendedSeeking, (void **) &m_pIAMExtendedSeeking)))
  {
    long chaptersCount = -1;
    m_pIAMExtendedSeeking->get_MarkerCount(&chaptersCount);
    if (chaptersCount <= 0)
    {
      SAFE_RELEASE(m_pIAMExtendedSeeking);
      return false;
    }

    SChapterInfos *infos = NULL;
    BSTR chapterName;
    for (int i = 1; i < chaptersCount + 1; i++)
    {
      infos = new SChapterInfos();
      infos->name = ""; infos->time = 0;

      if (SUCCEEDED(m_pIAMExtendedSeeking->GetMarkerName(i, &chapterName)))
      {
        g_charsetConverter.wToUTF8(chapterName, infos->name);
        SysFreeString(chapterName);
      } else
        infos->name = "Unknown chapter";

      m_pIAMExtendedSeeking->GetMarkerTime(i, &infos->time);

      infos->time *= 1000; // To ms      
      CLog::Log(LOGNOTICE, "%s Chapter \"%s\" found. Start time: %f", __FUNCTION__, infos->name.c_str(), infos->time);
      m_chapters.insert( std::pair<long, SChapterInfos *>(i, infos) );
    }

    m_currentChapter = 1;

    return true;
  } 
  else
  {
    CLog::Log(LOGERROR, "%s The splitter \"%s\" doesn't support chapters", __FUNCTION__, splitterName.c_str());
    return false;
  }
}

int CChaptersManager::SeekChapter(int iChapter)
{
  if (! m_init)
    return 0;

  if (GetChapterCount() > 0)
  {
    if (iChapter < 1)
      iChapter = 1;
    if (iChapter > GetChapterCount())
      return 0;

    // Seek to the chapter.
    CLog::Log(LOGDEBUG, "%s Seeking to chapter %d", __FUNCTION__, iChapter);
    m_pGraph->SeekInMilliSec( m_chapters[iChapter]->time );
  }
  else
  {
    // Do a regular big jump.
    if (iChapter > GetChapter())
      m_pGraph->Seek(true, true);
    else
      m_pGraph->Seek(false, true);
  }
  return 0;
}

void CChaptersManager::InitManager(CDSGraph *Graph)
{
  m_pGraph = Graph;
  m_init = true;
}

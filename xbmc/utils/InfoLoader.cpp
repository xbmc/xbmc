#include "../stdafx.h"
#include "InfoLoader.h"
#include "Weather.h"

CBackgroundLoader::CBackgroundLoader(CInfoLoader *callback)
{
  m_callback = callback;
  CThread::Create(true);
}

CBackgroundLoader::~CBackgroundLoader()
{
}

void CBackgroundLoader::Process()
{
  if (m_callback)
  {
    GetInformation();
    // and inform our callback that we're done
    m_callback->LoaderFinished();
  }
}

CInfoLoader::CInfoLoader(const char *type)
{
  m_refreshTime = 0;
  m_busy = true;
  m_backgroundLoader = NULL;
  m_type = type;
}

CInfoLoader::~CInfoLoader()
{
}

void CInfoLoader::Refresh()
{
  if (!m_backgroundLoader)
  {
    if (m_type == "weather")
      m_backgroundLoader = new CBackgroundWeatherLoader(this);
    if (!m_backgroundLoader)
    {
      CLog::Log(LOGERROR, "Unable to start the background %s loader", "weather");
      return;
    }
  }
  m_busy = true;
}

void CInfoLoader::LoaderFinished()
{
  m_refreshTime = timeGetTime() + TimeToNextRefreshInMs();
  m_backgroundLoader = NULL;
  m_busy = false;
}

const char *CInfoLoader::GetInfo(DWORD dwInfo)
{
  // Refresh if need be
  if (m_refreshTime < timeGetTime())
  {
    Refresh();
  }
  if (m_busy)
  {
    return BusyInfo(dwInfo);
  }
  return TranslateInfo(dwInfo);
}

const char *CInfoLoader::BusyInfo(DWORD dwInfo)
{
  m_busyText = g_localizeStrings.Get(503);
  return m_busyText.c_str();
}

const char *CInfoLoader::TranslateInfo(DWORD dwInfo)
{
  return "";
}

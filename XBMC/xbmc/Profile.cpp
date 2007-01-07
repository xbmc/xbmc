#include "stdafx.h"
#include ".\profile.h"
#include "utils/guiinfomanager.h"

CProfile::CProfile(void)
{
  _bDatabases = true;
  _bCanWrite = true;
  _bSources = true;
  _bCanWriteSources = true;
}

CProfile::~CProfile(void)
{}

void CProfile::setDate()
{
  CStdString strDate = g_infoManager.GetDate(true);
  CStdString strTime = g_infoManager.GetTime();
  if (strDate.IsEmpty() || strTime.IsEmpty())
    g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate("-");
  else
    g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate(strDate+" - "+strTime);
}
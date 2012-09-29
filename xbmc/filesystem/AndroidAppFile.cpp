/*
 *      Copyright (C) 2012 Team XBMC
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

#include "system.h"

#if defined(TARGET_ANDROID)

#include "AndroidAppFile.h"
#include <sys/stat.h>
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include <jni.h>
#include "android/activity/XBMCApp.h"
using namespace XFILE;

CFileAndroidApp::CFileAndroidApp(void)
{
  m_iconWidth = 0;
  m_iconHeight = 0;
}

CFileAndroidApp::~CFileAndroidApp(void)
{
  Close();
}

bool CFileAndroidApp::Open(const CURL& url)
{

  m_url = url;
  m_appname =  URIUtils::GetFileName(url.Get());
  m_appname = m_appname.Left(m_appname.size() - 4);

  return m_appname.size() > 0;
}

bool CFileAndroidApp::Exists(const CURL& url)
{
  return true;
}

unsigned int CFileAndroidApp::Read(void* lpBuf, int64_t uiBufSize)
{
  CXBMCApp::GetIcon(m_appname, lpBuf, uiBufSize);
  return uiBufSize;
}

void CFileAndroidApp::Close()
{
}

int64_t CFileAndroidApp::GetLength()
{
  CXBMCApp::GetIconSize(m_appname, &m_iconWidth, &m_iconHeight);
  return m_iconWidth * m_iconHeight * 4;
}

unsigned int CFileAndroidApp::GetIconWidth()
{
  return m_iconWidth;
}

unsigned int CFileAndroidApp::GetIconHeight()
{
  return m_iconHeight;
}

int CFileAndroidApp::GetChunkSize()
{
  return 0;
}
int CFileAndroidApp::Stat(const CURL& url, struct __stat64* buffer)
{
  return 0;
}
int CFileAndroidApp::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
    return 0;
  return 1;
}
#endif


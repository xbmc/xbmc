/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "system.h"

#if defined(TARGET_ANDROID)

#include "AndroidSettingFile.h"
#include <sys/stat.h>
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include <jni.h>
using namespace XFILE;

CFileAndroidSetting::CFileAndroidSetting(void)
{
  m_iconWidth = 0;
  m_iconHeight = 0;
}

CFileAndroidSetting::~CFileAndroidSetting(void)
{
  Close();
}

bool CFileAndroidSetting::Open(const CURL& url)
{
  return true;
}

bool CFileAndroidSetting::Exists(const CURL& url)
{
  return true;
}

void CFileAndroidSetting::Close()
{
}

int64_t CFileAndroidSetting::GetLength()
{
  return 0;
}

unsigned int CFileAndroidSetting::GetIconWidth()
{
  return m_iconWidth;
}

unsigned int CFileAndroidSetting::GetIconHeight()
{
  return m_iconHeight;
}

int CFileAndroidSetting::GetChunkSize()
{
  return 0;
}
int CFileAndroidSetting::Stat(const CURL& url, struct __stat64* buffer)
{
  return 0;
}
int CFileAndroidSetting::IoControl(EIoControl request, void* param)
{
  if(request == IOCTRL_SEEK_POSSIBLE)
    return 0;
  return 1;
}
#endif


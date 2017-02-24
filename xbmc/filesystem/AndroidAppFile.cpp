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

#if defined(TARGET_ANDROID)

#include "AndroidAppFile.h"
#include "platform/android/activity/XBMCApp.h"
#include <sys/stat.h>
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include <jni.h>
#include <android/bitmap.h>
#include "androidjni/Context.h"
#include "androidjni/Build.h"
#include "androidjni/DisplayMetrics.h"
#include "androidjni/Resources.h"
#include "androidjni/Bitmap.h"
#include "androidjni/BitmapDrawable.h"
#include "androidjni/PackageManager.h"
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
  m_packageName =  URIUtils::GetFileName(url.Get());
  m_packageName = m_packageName.substr(0, m_packageName.size() - 4);

  std::vector<androidPackage> applications = CXBMCApp::GetApplications();
  for(std::vector<androidPackage>::iterator i = applications.begin(); i != applications.end(); ++i)
  {
    if ((*i).packageName == m_packageName)
    {
      m_packageLabel = i->packageLabel;
      m_icon         = i->icon;
      return true;
    }
  }

  return false;
}

bool CFileAndroidApp::Exists(const CURL& url)
{
  std::string appname =  URIUtils::GetFileName(url.Get());
  appname = appname.substr(0, appname.size() - 4);

  std::vector<androidPackage> applications = CXBMCApp::GetApplications();
  for(std::vector<androidPackage>::iterator i = applications.begin(); i != applications.end(); ++i)
  {
    if ((*i).packageName == appname)
      return true;
  }

  return false;
}

unsigned int CFileAndroidApp::ReadIcon(unsigned char** lpBuf, unsigned int* width, unsigned int* height)
{
  JNIEnv* env = xbmc_jnienv();
  void *bitmapBuf = NULL;
  int densities[] = { CJNIDisplayMetrics::DENSITY_XXXHIGH, CJNIDisplayMetrics::DENSITY_XXHIGH, CJNIDisplayMetrics::DENSITY_XHIGH, -1 };

  CJNIBitmap bmp;
  if (CJNIBuild::SDK_INT >= 15 && m_icon)
  {
    CJNIResources res = CJNIContext::GetPackageManager().getResourcesForApplication(m_packageName);
    if (res)
    {
      for (int i=0; densities[i] != -1 && !bmp; ++i)
      {
        int density = densities[i];
        CJNIBitmapDrawable resbmp = res.getDrawableForDensity(m_icon, density);
        if (xbmc_jnienv()->ExceptionCheck())
          xbmc_jnienv()->ExceptionClear();
        else
          if (resbmp.getBitmap())
            bmp = resbmp.getBitmap();
      }
    }
  }

  if (!bmp)
    bmp = ((CJNIBitmapDrawable)(CJNIContext::GetPackageManager().getApplicationIcon(m_packageName))).getBitmap();
  if (!bmp)
    return 0;

  AndroidBitmapInfo info;
  AndroidBitmap_getInfo(env, bmp.get_raw(), &info);
  if (!info.width || !info.height)
    return 0;

  *width = info.width;
  *height = info.height;

  int imgsize = *width * *height * 4;
  *lpBuf = new unsigned char[imgsize];

  AndroidBitmap_lockPixels(env, bmp.get_raw(), &bitmapBuf);
  if (bitmapBuf)
  {
    memcpy(*lpBuf, bitmapBuf, imgsize);
    AndroidBitmap_unlockPixels(env, bmp.get_raw());
    return imgsize;
  }
  return 0;
}

void CFileAndroidApp::Close()
{
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


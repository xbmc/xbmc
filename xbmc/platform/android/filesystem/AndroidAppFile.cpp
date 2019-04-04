/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <jni.h>
#include <sys/stat.h>

#include <android/bitmap.h>
#include <androidjni/Bitmap.h>
#include <androidjni/Drawable.h>
#include <androidjni/BitmapDrawable.h>
#include <androidjni/Build.h>
#include <androidjni/Context.h>
#include <androidjni/DisplayMetrics.h>
#include <androidjni/PackageManager.h>
#include <androidjni/Resources.h>

#include "AndroidAppFile.h"
#include "platform/android/activity/XBMCApp.h"
#include "Util.h"
#include "URL.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
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
  jclass cBmpDrw = env->FindClass("android/graphics/drawable/BitmapDrawable");

  if (CJNIBuild::SDK_INT >= 15 && m_icon)
  {
    CJNIResources res = CJNIContext::GetPackageManager().getResourcesForApplication(m_packageName);
    if (res)
    {
      for (int i=0; densities[i] != -1 && !bmp; ++i)
      {
        int density = densities[i];
        CJNIDrawable drw = res.getDrawableForDensity(m_icon, density);
        if (xbmc_jnienv()->ExceptionCheck())
          xbmc_jnienv()->ExceptionClear();
        else if (!drw);
        else
        {
          if (env->IsInstanceOf(drw.get_raw(), cBmpDrw))
          {
            CJNIBitmapDrawable resbmp = drw;
            if (resbmp)
              bmp = resbmp.getBitmap();
          }
        }
      }
    }
  }

  if (!bmp)
  {
    CJNIDrawable drw = CJNIContext::GetPackageManager().getApplicationIcon(m_packageName);
    if (xbmc_jnienv()->ExceptionCheck())
      xbmc_jnienv()->ExceptionClear();
    else if (!drw);
    else
    {
      if (env->IsInstanceOf(drw.get_raw(), cBmpDrw))
      {
        CJNIBitmapDrawable resbmp = drw;
        if (resbmp)
          bmp = resbmp.getBitmap();
      }
    }
  }
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

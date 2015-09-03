/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistfribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distfributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "StageFrightInterface.h"
#include "StageFrightVideo.h"

#include "windowing/WindowingFactory.h"
#include "settings/AdvancedSettings.h"

void* create_stf(CApplication* application, KODI::MESSAGING::CApplicationMessenger* applicationMessenger, CWinSystemEGL* windowing, CAdvancedSettings* advsettings)
{
  return (void*)new CStageFrightVideo(application, applicationMessenger, windowing, advsettings);
}

void destroy_stf(void* stf)
{
  delete (CStageFrightVideo*)stf;
}

bool stf_Open(void* stf, CDVDStreamInfo &hints)
{
  return ((CStageFrightVideo*)stf)->Open(hints);
}

void stf_Dispose(void* stf)
{
  ((CStageFrightVideo*)stf)->Dispose();
}

int  stf_Decode(void* stf, uint8_t *pData, int iSize, double dts, double pts)
{
  return ((CStageFrightVideo*)stf)->Decode(pData, iSize, dts, pts);
}

void stf_Reset(void* stf)
{
  ((CStageFrightVideo*)stf)->Reset();
}

bool stf_GetPicture(void* stf, DVDVideoPicture *pDvdVideoPicture)
{
  return ((CStageFrightVideo*)stf)->GetPicture(pDvdVideoPicture);
}

bool stf_ClearPicture(void* stf, DVDVideoPicture* pDvdVideoPicture)
{
  return ((CStageFrightVideo*)stf)->ClearPicture(pDvdVideoPicture);
}

void stf_SetDropState(void* stf, bool bDrop)
{
  ((CStageFrightVideo*)stf)->SetDropState(bDrop);
}

void stf_SetSpeed(void* stf, int iSpeed)
{
  ((CStageFrightVideo*)stf)->SetSpeed(iSpeed);
}

void stf_LockBuffer(void* stf, EGLImageKHR eglimg)
{
  ((CStageFrightVideo*)stf)->LockBuffer(eglimg);
}

void stf_ReleaseBuffer(void* stf, EGLImageKHR eglimg)
{
  ((CStageFrightVideo*)stf)->ReleaseBuffer(eglimg);
}

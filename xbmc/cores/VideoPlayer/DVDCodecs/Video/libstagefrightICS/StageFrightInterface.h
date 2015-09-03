#pragma once

/*
 *      Copyright (C) 2013 Team XBMC
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

#include "DVDVideoCodec.h"

class CStageFrightVideo;
class CApplication;
class CWinSystemEGL;
class CAdvancedSettings;

namespace KODI
{
  namespace MESSAGING
  {
    class CApplicationMessenger;
  }
}

extern "C"
{
  void* create_stf(CApplication* application, KODI::MESSAGING::CApplicationMessenger* applicationMessenger, CWinSystemEGL* windowing, CAdvancedSettings* advsettings);
  void destroy_stf(void*);

  bool stf_Open(void*, CDVDStreamInfo &hints);
  void stf_Dispose(void*);
  int  stf_Decode(void*, uint8_t *pData, int iSize, double dts, double pts);
  void stf_Reset(void*);
  bool stf_GetPicture(void*, DVDVideoPicture *pDvdVideoPicture);
  bool stf_ClearPicture(void*, DVDVideoPicture* pDvdVideoPicture);
  void stf_SetDropState(void*, bool bDrop);
  void stf_SetSpeed(void*, int iSpeed);

  void stf_LockBuffer(void*, EGLImageKHR eglimg);
  void stf_ReleaseBuffer(void*, EGLImageKHR eglimg);
}

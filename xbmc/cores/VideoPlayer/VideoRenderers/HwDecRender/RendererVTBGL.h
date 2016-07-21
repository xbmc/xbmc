/*
 *      Copyright (C) 2007-2015 Team XBMC
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

#pragma once

#include "system.h"

#if defined(TARGET_DARWIN_OSX)

#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"

class CRendererVTB : public CLinuxRendererGL
{
public:
  CRendererVTB();
  virtual ~CRendererVTB();

  // Player functions
  virtual void AddVideoPictureHW(DVDVideoPicture &picture, int index);
  virtual void ReleaseBuffer(int idx);

  // Feature support
  virtual bool Supports(EINTERLACEMETHOD method);

  virtual EINTERLACEMETHOD AutoInterlaceMethod();

protected:
  virtual bool LoadShadersHook();

  // textures
  virtual bool UploadTexture(int index);
  virtual void DeleteTexture(int index);
  virtual bool CreateTexture(int index);
};

#endif

#ifndef LINUXRENDERERATI_RENDERER
#define LINUXRENDERERATI_RENDERER

/*
 *      Copyright (C) 2007-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "../../../guilib/Surface.h"
#include "../ffmpeg/DllSwScale.h"
#include "../ffmpeg/DllAvCodec.h"
#include "LinuxRendererGL.h"

#ifdef HAS_SDL_OPENGL

using namespace Surface;

class CLinuxRendererATI:public CLinuxRendererGL
{
public:
  CLinuxRendererATI(bool enableshaders=false);
  virtual ~CLinuxRendererATI();

  // Player functions
  virtual void         ReleaseImage(int source, bool preserve = false);
  virtual void         FlipPage(int source);
  virtual unsigned int PreInit();
  virtual void         UnInit();

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

protected:
  virtual bool CreateYV12Texture(int index, bool clear=true);
  virtual bool ValidateRenderTarget();
};

#endif

#endif


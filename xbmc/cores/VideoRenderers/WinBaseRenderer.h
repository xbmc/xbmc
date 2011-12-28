#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#if !defined(_LINUX) && !defined(HAS_GL) && defined(HAS_DS_PLAYER)


#include "guilib/GraphicContext.h"
#include "RenderFlags.h"
#include "BaseRenderer.h"
#include "guilib/D3DResource.h"
#include "settings/VideoSettings.h"
#include "RenderCapture.h"

class CBaseTexture;

namespace DXVA { class CProcessor; }
class IPaintCallback;
struct DVDVideoPicture;

#define AUTOSOURCE -1

class CWinBaseRenderer : public CBaseRenderer
{
public:
  CWinBaseRenderer(){};
  virtual ~CWinBaseRenderer(){};

  virtual void Update(bool bPauseDrawing) {};
  virtual void SetupScreenshot() {};
  virtual void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height) {};

  bool RenderCapture(CRenderCapture* capture) { return false; };

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, unsigned int format){ return false; };
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false) { return 0; };
  virtual void         ReleaseImage(int source, bool preserve = false) {};
  virtual bool         AddVideoPicture(DVDVideoPicture* picture) {return false;};
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y) { return 0; };
  virtual void         AddProcessor(DXVA::CProcessor* processor, int64_t id) {};
  virtual void         FlipPage(int source) {};
  virtual unsigned int PreInit() { return 0; };
  virtual void         UnInit() {};
  virtual void         Reset() {}; /* resets renderer after seek for example */
  virtual bool         IsConfigured() {return false;}
  
  virtual void         RegisterCallback(IPaintCallback *callback) {};
  virtual void         UnregisterCallback() {};
  virtual inline void  OnAfterPresent() {};
  // TODO:DIRECTX - implement these
  virtual bool         SupportsBrightness() { return false; };
  virtual bool         SupportsContrast() { return false; };
  virtual bool         SupportsGamma() { return false; };

  virtual bool         Supports(ERENDERFEATURE feature) = 0;
  virtual bool         Supports(EDEINTERLACEMODE mode) = 0;
  virtual bool         Supports(EINTERLACEMETHOD method) = 0;
  virtual bool         Supports(ESCALINGMETHOD method) = 0;

  virtual EINTERLACEMETHOD AutoInterlaceMethod() = 0;

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255) = 0;
};



#else
#include "LinuxRenderer.h"
#endif
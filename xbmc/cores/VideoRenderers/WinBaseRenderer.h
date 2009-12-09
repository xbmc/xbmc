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

#if !defined(_LINUX) && !defined(HAS_GL)


#include "GraphicContext.h"
#include "RenderFlags.h"
#include "BaseRenderer.h"
#include "D3DResource.h"
#include "settings/VideoSettings.h"

/* this defines what color translation coefficients */
#define CONF_FLAGS_YUVCOEF_MASK(a) ((a) & 0x07)
#define CONF_FLAGS_YUVCOEF_BT709 0x01
#define CONF_FLAGS_YUVCOEF_BT601 0x02
#define CONF_FLAGS_YUVCOEF_240M  0x03
#define CONF_FLAGS_YUVCOEF_EBU   0x04

#define CONF_FLAGS_YUV_FULLRANGE 0x08
#define CONF_FLAGS_FULLSCREEN    0x10

#ifdef MP_DIRECTRENDERING
#define NUM_BUFFERS 3
#else
#define NUM_BUFFERS 2
#endif

#define ALIGN(value, alignment) (((value)+((alignment)-1))&~((alignment)-1))
#define CLAMP(a, min, max) ((a) > (max) ? (max) : ( (a) < (min) ? (min) : a ))

#define AUTOSOURCE -1

#define IMAGE_FLAG_WRITING   0x01 /* image is in use after a call to GetImage, caller may be reading or writing */
#define IMAGE_FLAG_READING   0x02 /* image is in use after a call to GetImage, caller is only reading */
#define IMAGE_FLAG_DYNAMIC   0x04 /* image was allocated due to a call to GetImage */
#define IMAGE_FLAG_RESERVED  0x08 /* image is reserved, must be asked for specifically used to preserve images */

#define IMAGE_FLAG_INUSE (IMAGE_FLAG_WRITING | IMAGE_FLAG_READING | IMAGE_FLAG_RESERVED)


#define RENDER_FLAG_EVEN        0x01
#define RENDER_FLAG_ODD         0x02
#define RENDER_FLAG_BOTH (RENDER_FLAG_EVEN | RENDER_FLAG_ODD)
#define RENDER_FLAG_FIELDMASK   0x03

#define RENDER_FLAG_NOOSD       0x04 /* don't draw any osd */

/* these two flags will be used if we need to render same image twice (bob deinterlacing) */
#define RENDER_FLAG_NOLOCK      0x10   /* don't attempt to lock texture before rendering */
#define RENDER_FLAG_NOUNLOCK    0x20   /* don't unlock texture after rendering */

class CBaseTexture;

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

enum EFIELDSYNC
{
  FS_NONE,
  FS_ODD,
  FS_EVEN,
  FS_BOTH,
};


struct YUVRANGE
{
  int y_min, y_max;
  int u_min, u_max;
  int v_min, v_max;
};

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;

class CWinBaseRenderer : public CBaseRenderer
{
public:
  CWinBaseRenderer(){};
  virtual ~CWinBaseRenderer(){};

  virtual void Update(bool bPauseDrawing) {};
  virtual void SetupScreenshot() {};
  virtual void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height) {};

  // Player functions
  virtual bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags){ return false; };
  virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false) { return 0; };
  virtual void         ReleaseImage(int source, bool preserve = false) {};
  virtual unsigned int DrawSlice(unsigned char *src[], int stride[], int w, int h, int x, int y) { return 0; };
  virtual void         FlipPage(int source) {};
  virtual unsigned int PreInit() { return 0; };
  virtual void         UnInit() {};
  virtual void         Reset() {}; /* resets renderer after seek for example */
  virtual bool         IsConfigured() {return false;}

  virtual void         PaintVideoTexture(IDirect3DTexture9* videoTexture,IDirect3DSurface9* videoSurface) {};
  // TODO:DIRECTX - implement these
  virtual bool         SupportsBrightness() { return true; };
  virtual bool         SupportsContrast() { return true; };
  virtual bool         SupportsGamma() { return false; };
  virtual bool         Supports(EINTERLACEMETHOD method){ return false; };
  virtual bool         Supports(ESCALINGMETHOD method){ return false; };

  virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255) = 0;
};



#else
#include "LinuxRenderer.h"
#endif



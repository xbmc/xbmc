/*
 *      Copyright (C) 2007-2017 Team XBMC
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <GL/gl.h>
#include <GL/glext.h>

namespace VDPAU
{
class CVdpauRenderPicture;


struct InteropInfo
{
  PFNGLVDPAUINITNVPROC glVDPAUInitNV;
  PFNGLVDPAUFININVPROC glVDPAUFiniNV;
  PFNGLVDPAUREGISTEROUTPUTSURFACENVPROC glVDPAURegisterOutputSurfaceNV;
  PFNGLVDPAUREGISTERVIDEOSURFACENVPROC glVDPAURegisterVideoSurfaceNV;
  PFNGLVDPAUISSURFACENVPROC glVDPAUIsSurfaceNV;
  PFNGLVDPAUUNREGISTERSURFACENVPROC glVDPAUUnregisterSurfaceNV;
  PFNGLVDPAUSURFACEACCESSNVPROC glVDPAUSurfaceAccessNV;
  PFNGLVDPAUMAPSURFACESNVPROC glVDPAUMapSurfacesNV;
  PFNGLVDPAUUNMAPSURFACESNVPROC glVDPAUUnmapSurfacesNV;
  PFNGLVDPAUGETSURFACEIVNVPROC glVDPAUGetSurfaceivNV;
  GLenum textureTarget;
};

class CInteropState
{
public:
  bool Init(void *device, void *procFunc, void *ident);
  void Finish();
  InteropInfo &GetInterop();
  bool NeedInit(void *device, void *procFunc, void *ident);

protected:
  void *m_device = nullptr;
  void *m_procFunc = nullptr;
  void *m_ident = nullptr;
  InteropInfo m_interop;
};

class CVdpauTexture
{
public:
  bool Map(VDPAU::CVdpauRenderPicture *pic);
  void Unmap();
  void Init(InteropInfo &interop);

  GLuint m_texture = 0;
  GLuint m_textureTopY = 0;
  GLuint m_textureTopUV = 0;
  GLuint m_textureBotY = 0;
  GLuint m_textureBotUV = 0;
  int m_texWidth = 0;
  int m_texHeight = 0;

protected:
  bool MapNV12();
  void UnmapNV12();
  bool MapRGB();
  void UnmapRGB();
  InteropInfo m_interop;
  CVdpauRenderPicture *m_vdpauPic = nullptr;
  struct GLSurface
  {
    GLvdpauSurfaceNV glVdpauSurface;
  } m_glSurface;
};
}

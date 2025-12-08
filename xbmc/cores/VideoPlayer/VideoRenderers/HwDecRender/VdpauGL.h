/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// always define GL_GLEXT_PROTOTYPES before include gl headers
#if !defined(GL_GLEXT_PROTOTYPES)
#define GL_GLEXT_PROTOTYPES
#endif

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
  bool Init(void *device, void *procFunc, int64_t ident);
  void Finish();
  InteropInfo &GetInterop();
  bool NeedInit(void *device, void *procFunc, int64_t ident);

protected:
  void *m_device = nullptr;
  void *m_procFunc = nullptr;
  int64_t m_ident = 0;
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

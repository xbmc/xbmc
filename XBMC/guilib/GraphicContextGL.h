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

/*!
\file GraphicContext.h
\brief
*/

#ifndef GUILIB_GRAPHICCONTEXT_GL_H
#define GUILIB_GRAPHICCONTEXT_GL_H

#pragma once

#include "GraphicContext.h"

#ifdef HAS_GL

/*!
 \ingroup graphics
 \brief
 */
class CGraphicContextGL : public CGraphicContextBase
{
public:
  CGraphicContextGL(void);
  virtual ~CGraphicContextGL(void);

  virtual XBMC::DevicePtr Get3DDevice() { return NULL; }
  virtual void GetRenderVersion(int& maj, int& min);

  virtual bool ValidateSurface(Surface::CSurface* dest=NULL);
  virtual Surface::CSurface* InitializeSurface();

  virtual void BeginPaint(Surface::CSurface* dest=NULL, bool lock=true);
  virtual void EndPaint(Surface::CSurface* dest=NULL, bool lock=true);
  virtual void ReleaseCurrentContext(Surface::CSurface* dest=NULL);
  virtual void AcquireCurrentContext(Surface::CSurface* dest=NULL);
  virtual void DeleteThreadContext();

  virtual void SetFullScreenRoot(bool fs = true);
  virtual void SetVideoResolution(RESOLUTION &res, BOOL NeedZ = FALSE, bool forceClear = false);

  virtual void CaptureStateBlock();
  virtual void ApplyStateBlock();
  virtual void Clear();

  virtual void Flip();

  virtual void ApplyHardwareTransform();
  virtual void RestoreHardwareTransform();

protected:
  // set / get rendering api specific viewport
  virtual CRect GetRenderViewPort();
  virtual void SetRendrViewPort(CRect& viewPort);
  virtual void UpdateCameraPosition(const CPoint &camera);
};

extern CGraphicContextGL g_graphicsContext;

#endif

/*!
 \ingroup graphics
 \brief
 */
#endif

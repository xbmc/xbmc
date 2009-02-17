/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

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

#include "GUIControl.h"
#include "GUITexture.h"
#include "TextureManager.h"
#include "GUILabelControl.h"  // for CLabelInfo

// image alignment for <aspect>keep</aspect>, <aspect>scale</aspect> or <aspect>center</aspect>
#define ASPECT_ALIGN_CENTER  0
#define ASPECT_ALIGN_LEFT    1
#define ASPECT_ALIGN_RIGHT   2
#define ASPECT_ALIGNY_CENTER 0
#define ASPECT_ALIGNY_TOP    4
#define ASPECT_ALIGNY_BOTTOM 8
#define ASPECT_ALIGN_MASK    3
#define ASPECT_ALIGNY_MASK  ~3

class CImage
{
public:
  CImage(const CStdString &fileName) : file(fileName, "")
  {
    memset(&border, 0, sizeof(FRECT));
    orientation = 0;
  };

  CImage()
  {
    memset(&border, 0, sizeof(FRECT));
    orientation = 0;
  };

  void operator=(const CImage &left)
  {
    file = left.file;
    memcpy(&border, &left.border, sizeof(FRECT));
    orientation = left.orientation;
    diffuse = left.diffuse;
  };

  CTextureInfo GetInfo(bool useLarge) const
  {
    CTextureInfo info;
    memcpy(&info.border, &border, sizeof(FRECT));
    info.orientation = orientation;
    info.diffuse = diffuse;
    info.filename = file.GetLabel(0);
    info.useLarge = useLarge;
    return info;
  };

  CGUIInfoLabel file;
  FRECT      border;  // scaled  - unneeded if we get rid of scale on load
  int        orientation; // orientation of the texture (0 - 7 == EXIForientation - 1)
  CStdString diffuse; // diffuse overlay texture (unimplemented)
};

/*!
 \ingroup controls
 \brief 
 */

class CGUIImage : public CGUIControl
{
public:
  class CAspectRatio
  {
  public:
    enum ASPECT_RATIO { AR_STRETCH = 0, AR_SCALE, AR_KEEP, AR_CENTER };
    CAspectRatio(ASPECT_RATIO aspect = AR_STRETCH)
    {
      ratio = aspect;
      align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
      scaleDiffuse = true;
    };
    bool operator!=(const CAspectRatio &right) const
    {
      if (ratio != right.ratio) return true;
      if (align != right.align) return true;
      if (scaleDiffuse != right.scaleDiffuse) return true;
      return false;
    };

    ASPECT_RATIO ratio;
    DWORD        align;
    bool         scaleDiffuse;
  };

  CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, bool useLargeTexture = false);
  CGUIImage(const CGUIImage &left);
  virtual ~CGUIImage(void);
  virtual CGUIImage *Clone() const { return new CGUIImage(*this); };

  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;
  virtual bool IsAllocated() const;
  virtual void UpdateInfo(const CGUIListItem *item = NULL);

  virtual void SetFileName(const CStdString& strFileName, bool setConstant = false);
  virtual void SetAspectRatio(const CAspectRatio &aspect);
  void SetAspectRatio(CAspectRatio::ASPECT_RATIO ratio) { CAspectRatio aspect(ratio); SetAspectRatio(aspect); };
  void SetAlpha(unsigned char alpha);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetPosition(float posX, float posY);

  const CStdString& GetFileName() const;
  int GetTextureWidth() const;
  int GetTextureHeight() const;

  void CalculateSize();
#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  virtual void AllocateOnDemand();
  virtual void FreeTextures(bool immediately = false);
  void FreeResourcesButNotAnims();
  void Process();
  void Render(float left, float top, float bottom, float right, float u1, float v1, float u2, float v2);

  CAspectRatio m_aspect;
  bool m_bDynamicResourceAlloc;

  // for when we are changing textures
  bool m_texturesAllocated;

  //vertex values
  float m_fX;
  float m_fY;
  float m_fNW;
  float m_fNH;

  // border + conditional info
  CImage m_image;

  CGUITexture m_texture;
};
#endif

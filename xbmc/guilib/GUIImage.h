/*!
\file guiImage.h
\brief
*/

#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <vector>

#include "GUIControl.h"
#include "GUITexture.h"

/*!
 \ingroup controls
 \brief
 */

class CGUIImage : public CGUIControl
{
public:
  class CFadingTexture
  {
  public:
    CFadingTexture(const CGUITexture &texture, unsigned int fadeTime)
    {
      // create a copy of our texture, and allocate resources
      m_texture = new CGUITexture(texture);
      m_texture->AllocResources();
      m_fadeTime = fadeTime;
      m_fading = false;
    };
    ~CFadingTexture()
    {
      m_texture->FreeResources();
      delete m_texture;
    };

    CGUITexture *m_texture;  ///< texture to fade out
    unsigned int m_fadeTime; ///< time to fade out (ms)
    bool         m_fading;   ///< whether we're fading out

  private:
    CFadingTexture(const CFadingTexture&);
    CFadingTexture& operator=(const CFadingTexture&);
  };

  CGUIImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture);
  CGUIImage(const CGUIImage &left);
  ~CGUIImage(void) override;
  CGUIImage *Clone() const override { return new CGUIImage(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void UpdateVisibility(const CGUIListItem *item = NULL) override;
  bool OnAction(const CAction &action) override ;
  bool OnMessage(CGUIMessage& message) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  bool IsDynamicallyAllocated() override { return m_bDynamicResourceAlloc; };
  void SetInvalid() override;
  bool CanFocus() const override;
  void UpdateInfo(const CGUIListItem *item = NULL) override;

  virtual void SetInfo(const CGUIInfoLabel &info);
  virtual void SetFileName(const std::string& strFileName, bool setConstant = false, const bool useCache = true);
  virtual void SetAspectRatio(const CAspectRatio &aspect);
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  void SetPosition(float posX, float posY) override;
  std::string GetDescription() const override;
  void SetCrossFade(unsigned int time);

  const std::string& GetFileName() const;
  float GetTextureWidth() const;
  float GetTextureHeight() const;

  CRect CalcRenderRegion() const override;

#ifdef _DEBUG
  void DumpTextureUse() override;
#endif
protected:
  virtual void AllocateOnDemand();
  virtual void FreeTextures(bool immediately = false);
  void FreeResourcesButNotAnims();
  unsigned char GetFadeLevel(unsigned int time) const;
  bool ProcessFading(CFadingTexture *texture, unsigned int frameTime, unsigned int currentTime);

  bool m_bDynamicResourceAlloc;

  // border + conditional info
  CTextureInfo m_image;
  CGUIInfoLabel m_info;

  CGUITexture m_texture;
  std::vector<CFadingTexture *> m_fadingTextures;
  std::string m_currentTexture;
  std::string m_currentFallback;

  unsigned int m_crossFadeTime;
  unsigned int m_currentFadeTime;
  unsigned int m_lastRenderTime;
};
#endif

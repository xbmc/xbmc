/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/DirtyRegion.h"
#include "threads/CriticalSection.h"
#include "utils/ColorUtils.h"

#include <memory>
#include <string>

class CTexture;

class CSlideShowPic
{
public:
  enum DISPLAY_EFFECT { EFFECT_NONE = 0, EFFECT_FLOAT, EFFECT_ZOOM, EFFECT_RANDOM, EFFECT_PANORAMA, EFFECT_NO_TIMEOUT };
  enum TRANSITION_EFFECT { TRANSITION_NONE = 0, FADEIN_FADEOUT, CROSSFADE, TRANSITION_ZOOM, TRANSITION_ROTATE };

  struct TRANSITION
  {
    TRANSITION_EFFECT type = TRANSITION_NONE;
    int start = 0;
    int length = 0;
  };

  static std::unique_ptr<CSlideShowPic> CreateSlideShowPicture();

  CSlideShowPic();
  virtual ~CSlideShowPic();

  void SetTexture(int iSlideNumber,
                  std::unique_ptr<CTexture> pTexture,
                  DISPLAY_EFFECT dispEffect = EFFECT_RANDOM,
                  TRANSITION_EFFECT transEffect = FADEIN_FADEOUT);
  void UpdateTexture(std::unique_ptr<CTexture> pTexture);

  bool IsLoaded() const { return m_bIsLoaded; }
  void UnLoad() { m_bIsLoaded = false; }
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  void Render();
  void Close();
  void Reset(DISPLAY_EFFECT dispEffect = EFFECT_RANDOM, TRANSITION_EFFECT transEffect = FADEIN_FADEOUT);
  DISPLAY_EFFECT DisplayEffect() const { return m_displayEffect; }
  bool DisplayEffectNeedChange(DISPLAY_EFFECT newDispEffect) const;
  bool IsStarted() const { return m_iCounter > 0; }
  bool IsFinished() const { return m_bIsFinished; }
  bool DrawNextImage() const { return m_bDrawNextImage; }

  int GetWidth() const { return (int)m_fWidth; }
  int GetHeight() const { return (int)m_fHeight; }

  void Keep();
  bool StartTransition();
  int GetTransitionTime(int iType) const;
  void SetTransitionTime(int iType, int iTime);

  int SlideNumber() const { return m_iSlideNumber; }

  void Zoom(float fZoomAmount, bool immediate = false);
  void Rotate(float fRotateAngle, bool immediate = false);
  void Pause(bool bPause);
  void SetInSlideshow(bool slideshow);
  void SetOriginalSize(int iOriginalWidth, int iOriginalHeight, bool bFullSize);
  bool FullSize() const { return m_bFullSize; }
  int GetOriginalWidth();
  int GetOriginalHeight();

  void Move(float dX, float dY);
  float GetZoom() const { return m_fZoomAmount; }

  bool m_bIsComic;
  bool m_bCanMoveHorizontally;
  bool m_bCanMoveVertically;

protected:
  virtual void Render(float* x, float* y, CTexture* pTexture, UTILS::COLOR::Color color) = 0;

private:
  void SetTexture_Internal(int iSlideNumber,
                           std::unique_ptr<CTexture> pTexture,
                           DISPLAY_EFFECT dispEffect = EFFECT_RANDOM,
                           TRANSITION_EFFECT transEffect = FADEIN_FADEOUT);
  void UpdateVertices(float cur_x[4], float cur_y[4], const float new_x[4], const float new_y[4], CDirtyRegionList &dirtyregions);

  std::unique_ptr<CTexture> m_pImage;

  int m_iOriginalWidth;
  int m_iOriginalHeight;
  int m_iSlideNumber;
  bool m_bIsLoaded;
  bool m_bIsFinished;
  bool m_bDrawNextImage;
  bool m_bIsDirty;
  std::string m_strFileName;
  float m_fWidth;
  float m_fHeight;
  UTILS::COLOR::Color m_alpha = 0;
  // stuff relative to middle position
  float m_fPosX;
  float m_fPosY;
  float m_fPosZ;
  float m_fVelocityX;
  float m_fVelocityY;
  float m_fVelocityZ;
  float m_fZoomAmount;
  float m_fZoomLeft;
  float m_fZoomTop;
  float m_ax[4]{}, m_ay[4]{};
  float m_sx[4]{}, m_sy[4]{};
  float m_bx[4]{}, m_by[4]{};
  float m_ox[4]{}, m_oy[4]{};

  // transition and display effects
  DISPLAY_EFFECT m_displayEffect = EFFECT_NONE;
  TRANSITION m_transitionStart;
  TRANSITION m_transitionEnd;
  TRANSITION m_transitionTemp; // used for rotations + zooms
  float m_fAngle; // angle (between 0 and 2pi to display the image)
  float m_fTransitionAngle;
  float m_fTransitionZoom;
  int m_iCounter = 0;
  int m_iTotalFrames;
  bool m_bPause;
  bool m_bNoEffect;
  bool m_bFullSize;
  bool m_bTransitionImmediately;

  CCriticalSection m_textureAccess;
};

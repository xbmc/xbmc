/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "threads/CriticalSection.h"
#include "guilib/DirtyRegion.h"
#include "utils/Color.h"
#include <string>
#ifdef HAS_DX
#include "guilib/GUIShaderDX.h"
#include <wrl/client.h>
#endif

class CBaseTexture;

class CSlideShowPic
{
public:
  enum DISPLAY_EFFECT { EFFECT_NONE = 0, EFFECT_FLOAT, EFFECT_ZOOM, EFFECT_RANDOM, EFFECT_PANORAMA, EFFECT_NO_TIMEOUT };
  enum TRANSITION_EFFECT { TRANSITION_NONE = 0, FADEIN_FADEOUT, CROSSFADE, TRANSITION_ZOOM, TRANSITION_ROTATE };

  struct TRANSITION
  {
    TRANSITION_EFFECT type;
    int start;
    int length;
  };

  CSlideShowPic();
  ~CSlideShowPic();

  void SetTexture(int iSlideNumber, CBaseTexture* pTexture, DISPLAY_EFFECT dispEffect = EFFECT_RANDOM, TRANSITION_EFFECT transEffect = FADEIN_FADEOUT);
  void UpdateTexture(CBaseTexture* pTexture);

  bool IsLoaded() const { return m_bIsLoaded;};
  void UnLoad() {m_bIsLoaded = false;};
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  void Render();
  void Close();
  void Reset(DISPLAY_EFFECT dispEffect = EFFECT_RANDOM, TRANSITION_EFFECT transEffect = FADEIN_FADEOUT);
  DISPLAY_EFFECT DisplayEffect() const { return m_displayEffect; }
  bool DisplayEffectNeedChange(DISPLAY_EFFECT newDispEffect) const;
  bool IsStarted() const { return m_iCounter > 0; }
  bool IsFinished() const { return m_bIsFinished;};
  bool DrawNextImage() const { return m_bDrawNextImage;};

  int GetWidth() const { return (int)m_fWidth;};
  int GetHeight() const { return (int)m_fHeight;};

  void Keep();
  bool StartTransition();
  int GetTransitionTime(int iType) const;
  void SetTransitionTime(int iType, int iTime);

  int SlideNumber() const { return m_iSlideNumber;};

  void Zoom(float fZoomAmount, bool immediate = false);
  void Rotate(float fRotateAngle, bool immediate = false);
  void Pause(bool bPause);
  void SetInSlideshow(bool slideshow);
  void SetOriginalSize(int iOriginalWidth, int iOriginalHeight, bool bFullSize);
  bool FullSize() const { return m_bFullSize;};
  int GetOriginalWidth();
  int GetOriginalHeight();

  void Move(float dX, float dY);
  float GetZoom() const { return m_fZoomAmount;};

  bool m_bIsComic;
  bool m_bCanMoveHorizontally;
  bool m_bCanMoveVertically;
private:
  void SetTexture_Internal(int iSlideNumber, CBaseTexture* pTexture, DISPLAY_EFFECT dispEffect = EFFECT_RANDOM, TRANSITION_EFFECT transEffect = FADEIN_FADEOUT);
  void UpdateVertices(float cur_x[4], float cur_y[4], const float new_x[4], const float new_y[4], CDirtyRegionList &dirtyregions);
  void Render(float *x, float *y, CBaseTexture* pTexture, UTILS::Color color);
  CBaseTexture *m_pImage;

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
  UTILS::Color m_alpha = 0;
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
  float m_ax[4], m_ay[4];
  float m_sx[4], m_sy[4];
  float m_bx[4], m_by[4];
  float m_ox[4], m_oy[4];

  // transition and display effects
  DISPLAY_EFFECT m_displayEffect;
  TRANSITION m_transitionStart;
  TRANSITION m_transitionEnd;
  TRANSITION m_transitionTemp; // used for rotations + zooms
  float m_fAngle; // angle (between 0 and 2pi to display the image)
  float m_fTransitionAngle;
  float m_fTransitionZoom;
  int m_iCounter;
  int m_iTotalFrames;
  bool m_bPause;
  bool m_bNoEffect;
  bool m_bFullSize;
  bool m_bTransitionImmediately;

  CCriticalSection m_textureAccess;
#ifdef HAS_DX
  Microsoft::WRL::ComPtr<ID3D11Buffer> m_vb;
  bool UpdateVertexBuffer(Vertex *vertices);
#endif
};

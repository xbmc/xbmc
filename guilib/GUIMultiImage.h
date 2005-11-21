/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIMULTIIMAGECONTROL_H
#define GUILIB_GUIMULTIIMAGECONTROL_H

#pragma once

#include "GUIImage.h"
#include "../xbmc/lib/common/xbstopwatch.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIMultiImage : public CGUIControl
{
public:
  CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& texturePath, DWORD timePerImage, DWORD fadeTime);
  virtual ~CGUIMultiImage(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;

  void SetKeepAspectRatio(bool bOnOff);
  bool GetKeepAspectRatio() const;
  const CStdString &GetTexturePath() const { return m_texturePath; };
  DWORD GetTimePerImage() const { return m_timePerImage; };
  DWORD GetFadeTime() const { return m_fadeTime; };

protected:
  CStdString m_texturePath;
  int m_currentImage;
  CXBStopWatch m_imageTimer;
  CXBStopWatch m_fadeTimer;
  DWORD m_timePerImage;
  DWORD m_fadeTime;
  bool m_keepAspectRatio;
  vector <CGUIImage *> m_images;

  bool m_bDynamicResourceAlloc;
};
#endif

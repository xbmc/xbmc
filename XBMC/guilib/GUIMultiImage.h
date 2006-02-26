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
  CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& texturePath, DWORD timePerImage, DWORD fadeTime, bool randomized, bool loop);
  virtual ~CGUIMultiImage(void);

  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;

  void SetAspectRatio(CGUIImage::GUIIMAGE_ASPECT_RATIO ratio);
  CGUIImage::GUIIMAGE_ASPECT_RATIO GetAspectRatio() const;
  const CStdString &GetTexturePath() const { return m_texturePath; };
  DWORD GetTimePerImage() const { return m_timePerImage; };
  DWORD GetFadeTime() const { return m_fadeTime; };
  bool GetRandomized() const { return m_randomized; };
  bool GetLoop() const { return m_loop; };
  void SetInfo(int info) { m_Info = info; };
  int GetInfo() const { return m_Info; };

protected:
  void LoadDirectory();
  void LoadImage(int image);
  CStdString m_texturePath;
  int m_currentImage;
  CXBStopWatch m_imageTimer;
  CXBStopWatch m_fadeTimer;
  DWORD m_timePerImage;
  DWORD m_fadeTime;
  bool m_randomized;
  bool m_loop;
  CGUIImage::GUIIMAGE_ASPECT_RATIO m_aspectRatio;
  vector <CGUIImage *> m_images;

  bool m_bDynamicResourceAlloc;
  bool m_directoryLoaded;
  CStdStringArray m_files;
  int m_Info;
};
#endif

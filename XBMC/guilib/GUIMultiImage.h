/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIMULTIIMAGECONTROL_H
#define GUILIB_GUIMULTIIMAGECONTROL_H

#pragma once

#include "guiImage.h"
#include "../xbmc/utils/Stopwatch.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIMultiImage : public CGUIControl
{
public:
  CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CStdString& texturePath, DWORD timePerImage, DWORD fadeTime, bool randomized, bool loop, DWORD timeToPauseAtEnd);
  virtual ~CGUIMultiImage(void);

  virtual void Render();
  virtual void UpdateVisibility(void *pParam = NULL);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;

  void SetAspectRatio(CGUIImage::GUIIMAGE_ASPECT_RATIO ratio);
  void SetInfo(int info) { m_Info = info; };

protected:
  void LoadDirectory();
  void LoadImage(int image);
  CStdString m_texturePath;
  CStdString m_currentPath;
  unsigned int m_currentImage;
  CStopWatch m_imageTimer;
  CStopWatch m_fadeTimer;
  DWORD m_timePerImage;
  DWORD m_fadeTime;
  DWORD m_timeToPauseAtEnd;
  bool m_randomized;
  bool m_loop;
  CGUIImage::GUIIMAGE_ASPECT_RATIO m_aspectRatio;
  vector <CGUIImage *> m_images;

  bool m_bDynamicResourceAlloc;
  bool m_directoryLoaded;
  vector<CStdString> m_files;
  int m_Info;
};
#endif

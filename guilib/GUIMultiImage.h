/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIMULTIIMAGECONTROL_H
#define GUILIB_GUIMULTIIMAGECONTROL_H

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

#include "guiImage.h"
#include "utils/Stopwatch.h"

/*!
 \ingroup controls
 \brief 
 */
class CGUIMultiImage : public CGUIControl
{
public:
  CGUIMultiImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& texture, DWORD timePerImage, DWORD fadeTime, bool randomized, bool loop, DWORD timeToPauseAtEnd);
  CGUIMultiImage(const CGUIMultiImage &from);
  virtual ~CGUIMultiImage(void);
  virtual CGUIMultiImage *Clone() const { return new CGUIMultiImage(*this); };

  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;

  void SetInfo(const CGUIInfoLabel &info);
  void SetAspectRatio(const CAspectRatio &ratio);

protected:
  void LoadDirectory();
  void LoadImage(int image);
  CTextureInfo  m_textureInfo;
  CGUIInfoLabel m_texturePath;
  CStdString m_currentPath;
  unsigned int m_currentImage;
  CStopWatch m_imageTimer;
  CStopWatch m_fadeTimer;
  DWORD m_timePerImage;
  DWORD m_fadeTime;
  DWORD m_timeToPauseAtEnd;
  bool m_randomized;
  bool m_loop;
  CAspectRatio m_aspect;
  std::vector <CGUITexture *> m_images;

  bool m_bDynamicResourceAlloc;
  bool m_directoryLoaded;
  std::vector<CStdString> m_files;
};
#endif

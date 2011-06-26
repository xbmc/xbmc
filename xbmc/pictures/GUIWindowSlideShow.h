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

#include <set>
#include "guilib/GUIWindow.h"
#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "threads/Event.h"
#include "SlideShowPicture.h"
#include "DllImageLib.h"
#include "SortFileItem.h"

class CFileItemList;

class CGUIWindowSlideShow;

class CBackgroundPicLoader : public CThread
{
public:
  CBackgroundPicLoader();
  ~CBackgroundPicLoader();

  void Create(CGUIWindowSlideShow *pCallback);
  void LoadPic(int iPic, int iSlideNumber, const CStdString &strFileName, const int maxWidth, const int maxHeight);
  bool IsLoading() { return m_isLoading;};

private:
  void Process();
  int m_iPic;
  int m_iSlideNumber;
  CStdString m_strFileName;
  int m_maxWidth;
  int m_maxHeight;

  CEvent m_loadPic;
  bool m_isLoading;

  CGUIWindowSlideShow *m_pCallback;
};

class CGUIWindowSlideShow : public CGUIWindow
{
public:
  CGUIWindowSlideShow(void);
  virtual ~CGUIWindowSlideShow(void);

  void Reset();
  void Add(const CFileItem *picture);
  bool IsPlaying() const;
  void ShowNext();
  void ShowPrevious();
  void Select(const CStdString& strPicture);
  const CFileItemList &GetSlideShowContents();
  const CFileItemPtr GetCurrentSlide();
  void RunSlideShow(const CStdString &strPath, bool bRecursive = false,
                    bool bRandom = false, bool bNotRandom = false,
                    SORT_METHOD method = SORT_METHOD_LABEL,
                    SORT_ORDER order = SORT_ORDER_ASC, const CStdString &strExtensions="");
  void AddFromPath(const CStdString &strPath, bool bRecursive,
                   SORT_METHOD method=SORT_METHOD_LABEL, 
                   SORT_ORDER order=SORT_ORDER_ASC, const CStdString &strExtensions="");
  void StartSlideShow(bool screensaver=false);
  bool InSlideShow() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void FreeResources();
  void OnLoadPic(int iPic, int iSlideNumber, CBaseTexture* pTexture, int iOriginalWidth, int iOriginalHeight, bool bFullSize);
  int NumSlides() const;
  int CurrentSlide() const;
  void Shuffle();
private:
  typedef std::set<CStdString> path_set;  // set to track which paths we're adding
  void AddItems(const CStdString &strPath, path_set *recursivePaths,
                SORT_METHOD method=SORT_METHOD_LABEL,
                SORT_ORDER order=SORT_ORDER_ASC);
  void RenderPause();
  void RenderErrorMessage();
  void Rotate();
  void Zoom(int iZoom);
  void Move(float fX, float fY);
  void GetCheckedSize(float width, float height, int &maxWidth, int &maxHeight);
  int  GetNextSlide();

  int m_iCurrentSlide;
  int m_iNextSlide;
  int m_iDirection;
  int m_iRotate;
  int m_iZoomFactor;

  bool m_bSlideShow;
  bool m_bScreensaver;
  bool m_bPause;
  bool m_bPlayingVideo;
  bool m_bErrorMessage;

  CFileItemList* m_slides;

  CSlideShowPic m_Image[2];

  int m_iCurrentPic;
  // background loader
  CBackgroundPicLoader* m_pBackgroundLoader;
  bool m_bWaitForNextPic;
  bool m_bLoadNextPic;
  bool m_bReloadImage;
  DllImageLib m_ImageLib;
  RESOLUTION m_Resolution;
  CCriticalSection m_slideSection;
  CStdString m_strExtensions;
};

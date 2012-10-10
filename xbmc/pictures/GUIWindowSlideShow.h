#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
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
class CVariant;

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
  void GetSlideShowContents(CFileItemList &list);
  const CFileItemPtr GetCurrentSlide();
  void RunSlideShow(const CStdString &strPath, bool bRecursive = false,
                    bool bRandom = false, bool bNotRandom = false,
                    SORT_METHOD method = SORT_METHOD_LABEL,
                    SortOrder order = SortOrderAscending, const CStdString &strExtensions="");
  void AddFromPath(const CStdString &strPath, bool bRecursive,
                   SORT_METHOD method=SORT_METHOD_LABEL, 
                   SortOrder order = SortOrderAscending, const CStdString &strExtensions="");
  void StartSlideShow(bool screensaver=false);
  bool InSlideShow() const;
  virtual bool OnMessage(CGUIMessage& message);
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);  
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  virtual void Process(unsigned int currentTime, CDirtyRegionList &regions);
  virtual void OnDeinitWindow(int nextWindowID);
  void OnLoadPic(int iPic, int iSlideNumber, CBaseTexture* pTexture, bool bFullSize);
  int NumSlides() const;
  int CurrentSlide() const;
  void Shuffle();
  bool IsPaused() const { return m_bPause; }
  bool IsShuffled() const { return m_bShuffled; }
  int GetDirection() const { return m_iDirection; }
private:
  typedef std::set<CStdString> path_set;  // set to track which paths we're adding
  void AddItems(const CStdString &strPath, path_set *recursivePaths,
                SORT_METHOD method = SORT_METHOD_LABEL,
                SortOrder order = SortOrderAscending);
  void RenderPause();
  void RenderErrorMessage();
  void Rotate(float fAngle, bool immediate = false);
  void Zoom(int iZoom);
  void ZoomRelative(float fZoom, bool immediate = false);
  void Move(float fX, float fY);
  void GetCheckedSize(float width, float height, int &maxWidth, int &maxHeight);
  int  GetNextSlide();

  void AnnouncePlayerPlay(const CFileItemPtr& item);
  void AnnouncePlayerPause(const CFileItemPtr& item);
  void AnnouncePlayerStop(const CFileItemPtr& item);
  void AnnouncePlaylistRemove(int pos);
  void AnnouncePlaylistClear();
  void AnnouncePlaylistAdd(const CFileItemPtr& item, int pos);
  void AnnouncePropertyChanged(const std::string &strProperty, const CVariant &value);

  int m_iCurrentSlide;
  int m_iNextSlide;
  int m_iDirection;
  float m_fRotate;
  float m_fInitialRotate;
  int m_iZoomFactor;
  float m_fZoom;
  float m_fInitialZoom;

  bool m_bShuffled;
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
  CPoint m_firstGesturePoint;
};

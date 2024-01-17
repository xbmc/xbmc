/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "SlideShowPicture.h"
#include "guilib/GUIDialog.h"
#include "interfaces/ISlideShowDelegate.h"
#include "threads/Event.h"
#include "threads/Thread.h"

#include <memory>
#include <set>

class CFileItemList;
class CVariant;

class CGUIWindowSlideShow;

class CBackgroundPicLoader : public CThread
{
public:
  CBackgroundPicLoader();
  ~CBackgroundPicLoader() override;

  void Create(CGUIWindowSlideShow *pCallback);
  void LoadPic(int iPic, int iSlideNumber, const std::string &strFileName, const int maxWidth, const int maxHeight);
  bool IsLoading() { return m_isLoading; }
  int SlideNumber() const { return m_iSlideNumber; }
  int Pic() const { return m_iPic; }

private:
  void Process() override;
  int m_iPic = 0;
  int m_iSlideNumber = 0;
  std::string m_strFileName;
  int m_maxWidth = 0;
  int m_maxHeight = 0;

  CEvent m_loadPic;
  bool m_isLoading = false;

  CGUIWindowSlideShow* m_pCallback = nullptr;
};

class CGUIWindowSlideShow : public CGUIDialog, public ISlideShowDelegate
{
public:
  CGUIWindowSlideShow(void);
  ~CGUIWindowSlideShow() override;

  // Implementation of ISlideShowDelegate
  void Add(const CFileItem* picture) override;
  bool IsPlaying() const override;
  void Select(const std::string& picture) override;
  void GetSlideShowContents(CFileItemList& list) override;
  std::shared_ptr<const CFileItem> GetCurrentSlide() override;
  void StartSlideShow() override;
  void PlayPicture() override;
  bool InSlideShow() const override;
  int NumSlides() const override;
  int CurrentSlide() const override;
  bool IsPaused() const override { return m_bPause; }
  bool IsShuffled() const override { return m_bShuffled; }
  void Reset() override;
  void RunSlideShow(const std::string& strPath,
                    bool bRecursive = false,
                    bool bRandom = false,
                    bool bNotRandom = false,
                    const std::string& beginSlidePath = "",
                    bool startSlideShow = true,
                    SortBy method = SortByLabel,
                    SortOrder order = SortOrderAscending,
                    SortAttribute sortAttributes = SortAttributeNone,
                    const std::string& strExtensions = "") override;
  void AddFromPath(const std::string& strPath,
                   bool bRecursive,
                   SortBy method = SortByLabel,
                   SortOrder order = SortOrderAscending,
                   SortAttribute sortAttributes = SortAttributeNone,
                   const std::string& strExtensions = "") override;
  void Shuffle() override;
  int GetDirection() const override { return m_iDirection; }

  bool OnMessage(CGUIMessage& message) override;
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  bool OnAction(const CAction& action) override;
  void Render() override;
  void RenderEx() override;
  void Process(unsigned int currentTime, CDirtyRegionList& regions) override;
  void OnDeinitWindow(int nextWindowID) override;

  void OnLoadPic(int iPic,
                 int iSlideNumber,
                 const std::string& strFileName,
                 std::unique_ptr<CTexture> pTexture,
                 bool bFullSize);

  static void RunSlideShow(const std::vector<std::string>& paths, int start = 0);

private:
  void ShowNext();
  void ShowPrevious();
  void SetDirection(int direction); // -1: rewind, 1: forward

  typedef std::set<std::string> path_set;  // set to track which paths we're adding
  void AddItems(const std::string &strPath, path_set *recursivePaths,
                SortBy method = SortByLabel,
                SortOrder order = SortOrderAscending,
                SortAttribute sortAttributes = SortAttributeNone);
  bool PlayVideo();
  CSlideShowPic::DISPLAY_EFFECT GetDisplayEffect(int iSlideNumber) const;
  void RenderPause();
  void RenderErrorMessage();
  void Rotate(float fAngle, bool immediate = false);
  void Zoom(int iZoom);
  void ZoomRelative(float fZoom, bool immediate = false);
  void Move(float fX, float fY);
  void GetCheckedSize(float width, float height, int &maxWidth, int &maxHeight);
  std::string GetPicturePath(CFileItem *item);
  int  GetNextSlide();

  void AnnouncePlayerPlay(const CFileItemPtr& item);
  void AnnouncePlayerPause(const CFileItemPtr& item);
  void AnnouncePlayerStop(const CFileItemPtr& item);
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
  bool m_bPause;
  bool m_bPlayingVideo;
  int m_iVideoSlide = -1;
  bool m_bErrorMessage;

  std::vector<CFileItemPtr> m_slides;

  std::unique_ptr<CSlideShowPic> m_Image[2];

  int m_iCurrentPic;
  // background loader
  std::unique_ptr<CBackgroundPicLoader> m_pBackgroundLoader;
  int m_iLastFailedNextSlide;
  bool m_bLoadNextPic;
  RESOLUTION m_Resolution;
  CPoint m_firstGesturePoint;
};

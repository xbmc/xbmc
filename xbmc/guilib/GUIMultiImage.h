/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIMultiImage.h
\brief
*/

#include "GUIImage.h"
#include "threads/CriticalSection.h"
#include "utils/Job.h"
#include "utils/Stopwatch.h"

#include <vector>

/*!
 \ingroup controls
 \brief
 */
class CGUIMultiImage : public CGUIControl, public IJobCallback
{
public:
  CGUIMultiImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, unsigned int timePerImage, unsigned int fadeTime, bool randomized, bool loop, unsigned int timeToPauseAtEnd);
  CGUIMultiImage(const CGUIMultiImage &from);
  ~CGUIMultiImage(void) override;
  CGUIMultiImage* Clone() const override { return new CGUIMultiImage(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void UpdateVisibility(const CGUIListItem *item = NULL) override;
  void UpdateInfo(const CGUIListItem *item = NULL) override;
  bool OnAction(const CAction &action) override;
  bool OnMessage(CGUIMessage &message) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  bool IsDynamicallyAllocated() override { return m_bDynamicResourceAlloc; }
  void SetInvalid() override;
  bool CanFocus() const override;
  std::string GetDescription() const override;

  void SetInfo(const KODI::GUILIB::GUIINFO::CGUIInfoLabel &info);
  void SetAspectRatio(const CAspectRatio &ratio);

protected:
  void LoadDirectory();
  void OnDirectoryLoaded();
  void CancelLoading();
  void ResetMultiImage();

  enum DIRECTORY_STATUS { UNLOADED = 0, LOADING, LOADED, READY };
  void OnJobComplete(unsigned int jobID, bool success, CJob *job) override;

  class CMultiImageJob : public CJob
  {
  public:
    explicit CMultiImageJob(const std::string &path);
    bool DoWork() override;
    const char* GetType() const override { return "multiimage"; }

    std::vector<std::string> m_files;
    std::string              m_path;
  };

  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_texturePath;
  std::string m_currentPath;
  unsigned int m_currentImage;
  CStopWatch m_imageTimer;
  unsigned int m_timePerImage;
  unsigned int m_timeToPauseAtEnd;
  bool m_randomized;
  bool m_loop;

  bool m_bDynamicResourceAlloc;
  std::vector<std::string> m_files;

  CGUIImage m_image;

  CCriticalSection m_section;
  DIRECTORY_STATUS m_directoryStatus;
  unsigned int m_jobID;
};


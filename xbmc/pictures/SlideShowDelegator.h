/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/ISlideShowDelegate.h"

#include <memory>
#include <string>
#include <vector>

class CSlideShowDelegator : public ISlideShowDelegate
{
public:
  CSlideShowDelegator() = default;
  ~CSlideShowDelegator() = default;

  void SetDelegate(ISlideShowDelegate* delegate);
  void ResetDelegate();

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
  bool IsPaused() const override;
  bool IsShuffled() const override;
  void Reset() override;
  void Shuffle() override;
  int GetDirection() const override;
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

private:
  ISlideShowDelegate* m_delegate;
};

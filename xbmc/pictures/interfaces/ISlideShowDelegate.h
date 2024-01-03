/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/SortUtils.h"

#include <memory>

class CFileItem;
class CFileItemList;

class ISlideShowDelegate
{
public:
  ISlideShowDelegate() = default;
  virtual ~ISlideShowDelegate() = default;

  virtual void Add(const CFileItem* picture) = 0;
  virtual bool IsPlaying() const = 0;
  virtual void Select(const std::string& picture) = 0;
  virtual void GetSlideShowContents(CFileItemList& list) = 0;
  virtual std::shared_ptr<const CFileItem> GetCurrentSlide() = 0;
  virtual void StartSlideShow() = 0;
  virtual void PlayPicture() = 0;
  virtual bool InSlideShow() const = 0;
  virtual int NumSlides() const = 0;
  virtual int CurrentSlide() const = 0;
  virtual bool IsPaused() const = 0;
  virtual bool IsShuffled() const = 0;
  virtual void Reset() = 0;
  virtual void Shuffle() = 0;
  virtual int GetDirection() const = 0;
  //! @todo - refactor to use an options struct. Methods with so many arguments are a sign of a bad design...
  virtual void RunSlideShow(const std::string& strPath,
                            bool bRecursive = false,
                            bool bRandom = false,
                            bool bNotRandom = false,
                            const std::string& beginSlidePath = "",
                            bool startSlideShow = true,
                            SortBy method = SortByLabel,
                            SortOrder order = SortOrderAscending,
                            SortAttribute sortAttributes = SortAttributeNone,
                            const std::string& strExtensions = "") = 0;
  //! @todo - refactor to use an options struct. Methods with so many arguments are a sign of a bad design...
  virtual void AddFromPath(const std::string& strPath,
                           bool bRecursive,
                           SortBy method = SortByLabel,
                           SortOrder order = SortOrderAscending,
                           SortAttribute sortAttributes = SortAttributeNone,
                           const std::string& strExtensions = "") = 0;
};

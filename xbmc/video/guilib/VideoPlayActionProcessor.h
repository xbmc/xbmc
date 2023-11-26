/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoPlayAction.h"

#include <memory>

class CFileItem;

namespace VIDEO
{
namespace GUILIB
{
class CVideoPlayActionProcessorBase
{
public:
  explicit CVideoPlayActionProcessorBase(const std::shared_ptr<CFileItem>& item) : m_item(item) {}
  virtual ~CVideoPlayActionProcessorBase() = default;

  static PlayAction GetDefaultPlayAction();

  bool Process();
  bool Process(PlayAction playAction);

  bool UserCancelled() const { return m_userCancelled; }

protected:
  virtual bool OnResumeSelected() = 0;
  virtual bool OnPlaySelected() = 0;

  std::shared_ptr<CFileItem> m_item;
  bool m_userCancelled{false};

private:
  CVideoPlayActionProcessorBase() = delete;
  PlayAction ChoosePlayOrResume();
};
} // namespace GUILIB
} // namespace VIDEO

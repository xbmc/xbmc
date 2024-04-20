/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoAction.h"

#include <memory>

class CFileItem;

namespace KODI::VIDEO::GUILIB
{
class CVideoPlayActionProcessorBase
{
public:
  explicit CVideoPlayActionProcessorBase(const std::shared_ptr<CFileItem>& item) : m_item(item) {}
  virtual ~CVideoPlayActionProcessorBase() = default;

  bool ProcessDefaultAction();
  bool ProcessAction(Action action);

  bool UserCancelled() const { return m_userCancelled; }

  static Action ChoosePlayOrResume(const CFileItem& item);

protected:
  virtual Action GetDefaultAction();
  virtual bool Process(Action action);

  virtual bool OnResumeSelected() = 0;
  virtual bool OnPlaySelected() = 0;

  std::shared_ptr<CFileItem> m_item;
  bool m_userCancelled{false};

private:
  CVideoPlayActionProcessorBase() = delete;
};
} // namespace KODI::VIDEO::GUILIB

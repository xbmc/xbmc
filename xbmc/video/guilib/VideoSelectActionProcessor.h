/*
 *  Copyright (C) 2023-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoActionProcessorBase.h"

#include <memory>

class CFileItem;

namespace KODI::VIDEO::GUILIB
{
class CVideoSelectActionProcessor : public CVideoActionProcessorBase
{
public:
  explicit CVideoSelectActionProcessor(const std::shared_ptr<CFileItem>& item)
    : CVideoActionProcessorBase(item)
  {
  }

  ~CVideoSelectActionProcessor() override = default;

protected:
  Action GetDefaultAction() override;
  bool Process(Action action) override;

  virtual bool OnPlaySelected();
  virtual bool OnQueueSelected();
  virtual bool OnInfoSelected();
  virtual bool OnChooseSelected();

private:
  CVideoSelectActionProcessor() = delete;
};
} // namespace KODI::VIDEO::GUILIB

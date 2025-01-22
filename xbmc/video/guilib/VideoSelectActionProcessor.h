/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoPlayActionProcessor.h"

#include <memory>

class CFileItem;

namespace KODI::VIDEO::GUILIB
{
class CVideoSelectActionProcessor : public CVideoPlayActionProcessor
{
public:
  explicit CVideoSelectActionProcessor(const std::shared_ptr<CFileItem>& item)
    : CVideoPlayActionProcessor(item)
  {
  }

  ~CVideoSelectActionProcessor() override = default;

  static Action GetDefaultSelectAction();

protected:
  Action GetDefaultAction() override;
  bool Process(Action action) override;

  virtual bool OnQueueSelected();
  virtual bool OnInfoSelected();
  virtual bool OnChooseSelected();

private:
  CVideoSelectActionProcessor() = delete;
};
} // namespace KODI::VIDEO::GUILIB

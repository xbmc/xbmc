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
class CVideoSelectActionProcessorBase : public CVideoPlayActionProcessorBase
{
public:
  explicit CVideoSelectActionProcessorBase(const std::shared_ptr<CFileItem>& item)
    : CVideoPlayActionProcessorBase(item)
  {
  }

  ~CVideoSelectActionProcessorBase() override = default;

  static Action GetDefaultSelectAction();

protected:
  Action GetDefaultAction() override;
  bool Process(Action action) override;

  virtual bool OnPlayPartSelected(unsigned int part) = 0;
  virtual bool OnQueueSelected() = 0;
  virtual bool OnInfoSelected() = 0;
  virtual bool OnChooseSelected() = 0;

private:
  CVideoSelectActionProcessorBase() = delete;
  unsigned int ChooseStackItemPartNumber() const;
};
} // namespace KODI::VIDEO::GUILIB

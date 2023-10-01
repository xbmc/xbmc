/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoPlayAction.h"

class CFileItem;

namespace VIDEO
{
namespace GUILIB
{
class CVideoPlayActionProcessorBase
{
public:
  explicit CVideoPlayActionProcessorBase(CFileItem& item) : m_item(item) {}
  virtual ~CVideoPlayActionProcessorBase() = default;

  static PlayAction GetDefaultPlayAction();

  bool Process();
  bool Process(PlayAction playAction);

protected:
  virtual bool OnResumeSelected() = 0;
  virtual bool OnPlaySelected() = 0;

  CFileItem& m_item;

private:
  CVideoPlayActionProcessorBase() = delete;
  PlayAction ChoosePlayOrResume();
};
} // namespace GUILIB
} // namespace VIDEO

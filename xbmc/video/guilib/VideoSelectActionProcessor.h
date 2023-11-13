/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "video/guilib/VideoSelectAction.h"

class CFileItem;

namespace VIDEO
{
namespace GUILIB
{
class CVideoSelectActionProcessorBase
{
public:
  explicit CVideoSelectActionProcessorBase(CFileItem& item) : m_item(item) {}
  virtual ~CVideoSelectActionProcessorBase() = default;

  static SelectAction GetDefaultSelectAction();

  bool Process();
  bool Process(SelectAction selectAction);

  static SelectAction ChoosePlayOrResume(const CFileItem& item);

protected:
  virtual bool OnPlayPartSelected(unsigned int part) = 0;
  virtual bool OnResumeSelected() = 0;
  virtual bool OnPlaySelected() = 0;
  virtual bool OnQueueSelected() = 0;
  virtual bool OnInfoSelected() = 0;
  virtual bool OnMoreSelected() = 0;

  CFileItem& m_item;

private:
  CVideoSelectActionProcessorBase() = delete;
  SelectAction ChooseVideoItemSelectAction() const;
  unsigned int ChooseStackItemPartNumber() const;
};
} // namespace GUILIB
} // namespace VIDEO

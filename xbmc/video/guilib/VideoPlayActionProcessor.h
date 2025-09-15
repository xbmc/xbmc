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
class CVideoPlayActionProcessor : public CVideoActionProcessorBase
{
public:
  using CVideoActionProcessorBase::CVideoActionProcessorBase;

  void SetChoosePlayer() { m_choosePlayer = true; }
  static Action ChoosePlayOrResume(const CFileItem& item);

protected:
  Action GetDefaultAction() override;
  bool Process(Action action) override;

  virtual bool OnResumeSelected();
  virtual bool OnPlaySelected();

private:
  CVideoPlayActionProcessor() = delete;
  unsigned int ChooseStackPart() const;
  Action ChoosePlayOrResume() const;
  static Action ChoosePlayOrResume(const std::string& resumeString);
  void SetResumeData();
  void SetStartData();
  void Play(const std::string& player);

  bool m_choosePlayer{false};
};
} // namespace KODI::VIDEO::GUILIB

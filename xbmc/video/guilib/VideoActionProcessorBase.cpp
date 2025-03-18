/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoActionProcessorBase.h"

#include "video/guilib/VideoVersionHelper.h"

namespace KODI::VIDEO::GUILIB
{

bool CVideoActionProcessorBase::ProcessDefaultAction()
{
  return ProcessAction(GetDefaultAction());
}

bool CVideoActionProcessorBase::ProcessAction(Action action)
{
  m_userCancelled = false;

  const auto movie{CVideoVersionHelper::ChooseVideoFromAssets(m_item)};
  if (movie)
  {
    m_item = movie;
  }
  else
  {
    m_userCancelled = true;
    return true; // User cancelled the select menu. We're done.
  }

  return Process(action);
}

} // namespace KODI::VIDEO::GUILIB

/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "threads/CriticalSection.h"

#include <set>

class IGUIVolumeBarCallback;

class CGUIDialogVolumeBar : public CGUIDialog
{
public:
  CGUIDialogVolumeBar(void);
  ~CGUIDialogVolumeBar(void) override;
  bool OnMessage(CGUIMessage& message) override;
  bool OnAction(const CAction &action) override;

  // Volume bar interface
  void RegisterCallback(IGUIVolumeBarCallback *callback);
  void UnregisterCallback(IGUIVolumeBarCallback *callback);
  bool IsVolumeBarEnabled() const;

private:
  std::set<IGUIVolumeBarCallback*> m_callbacks;
  mutable CCriticalSection m_callbackMutex;
};

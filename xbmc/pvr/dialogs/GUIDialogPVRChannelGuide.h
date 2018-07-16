/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#include "view/GUIViewControl.h"

#include "pvr/PVRTypes.h"

class CFileItemList;

namespace PVR
{
  class CGUIDialogPVRChannelGuide : public CGUIDialog
  {
  public:
    CGUIDialogPVRChannelGuide(void);
    ~CGUIDialogPVRChannelGuide(void) override;
    bool OnMessage(CGUIMessage& message) override;
    void OnWindowLoaded() override;
    void OnWindowUnload() override;

    void Open(const CPVRChannelPtr &channel);

  protected:
    void OnInitWindow() override;
    void OnDeinitWindow(int nextWindowID) override;

    CGUIControl *GetFirstFocusableControl(int id) override;

    std::unique_ptr<CFileItemList> m_vecItems;
    CGUIViewControl m_viewControl;

  private:
    void ShowInfo(int iItem);
    void Clear();

    CPVRChannelPtr m_channel;
  };
}

/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"

#include "pvr/PVRTypes.h"

namespace PVR
{
  class CGUIDialogPVRGuideInfo : public CGUIDialog
  {
  public:
    CGUIDialogPVRGuideInfo(void);
    ~CGUIDialogPVRGuideInfo(void) override;
    bool OnMessage(CGUIMessage& message) override;
    bool OnInfo(int actionID) override;
    bool HasListItems() const override { return true; }
    CFileItemPtr GetCurrentListItem(int offset = 0) override;

    void SetProgInfo(const CPVREpgInfoTagPtr &tag);

    static void ShowFor(const CFileItemPtr& item);

  protected:
    void OnInitWindow() override;

  private:
    bool OnClickButtonOK(CGUIMessage &message);
    bool OnClickButtonRecord(CGUIMessage &message);
    bool OnClickButtonPlay(CGUIMessage &message);
    bool OnClickButtonFind(CGUIMessage &message);
    bool OnClickButtonAddTimer(CGUIMessage &message);

    CPVREpgInfoTagPtr m_progItem;
  };
}

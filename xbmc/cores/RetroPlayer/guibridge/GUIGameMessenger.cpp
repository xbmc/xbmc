/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameMessenger.h"

#include "FileItem.h"
#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "cores/RetroPlayer/savestates/SavestateDatabase.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"

using namespace KODI;
using namespace RETRO;

CGUIGameMessenger::CGUIGameMessenger(CRPProcessInfo& processInfo)
  : m_guiComponent(processInfo.GetRenderContext().GUI())
{
}

void CGUIGameMessenger::RefreshSavestates(const std::string& savestatePath /* = "" */,
                                          ISavestate* savestate /* = nullptr */)
{
  if (m_guiComponent != nullptr)
  {
    CGUIMessage message(GUI_MSG_REFRESH_THUMBS, 0, WINDOW_DIALOG_IN_GAME_SAVES);

    // Add path, if given
    if (!savestatePath.empty())
      message.SetStringParam(savestatePath);

    // Add savestate info, if given
    if (savestate != nullptr)
    {
      CFileItemPtr item = std::make_shared<CFileItem>();
      CSavestateDatabase::GetSavestateItem(*savestate, savestatePath, *item);
      message.SetItem(std::static_pointer_cast<CGUIListItem>(item));
    }

    // Notify the in-game savestate dialog
    m_guiComponent->GetWindowManager().SendThreadMessage(message, WINDOW_DIALOG_IN_GAME_SAVES);
  }
}

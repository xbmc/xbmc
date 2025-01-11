/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIBuiltinsUtils.h"

#include "FileItem.h"
#include "ServiceBroker.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "utils/ExecString.h"

#include <string>

namespace KODI::UTILS::GUILIB
{
bool CGUIBuiltinsUtils::ExecuteAction(const CExecString& execute,
                                      const std::shared_ptr<CFileItem>& item)
{
  return ExecuteAction(execute.GetExecString(), item);
}

bool CGUIBuiltinsUtils::ExecuteAction(const std::string& execute,
                                      const std::shared_ptr<CFileItem>& item)
{
  if (!execute.empty())
  {
    CGUIMessage message(GUI_MSG_EXECUTE, 0, 0);
    message.SetStringParam(execute);
    message.SetItem(item);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
    return true;
  }
  return false;
}

bool CGUIBuiltinsUtils::ExecutePlayMediaTryResume(const std::shared_ptr<CFileItem>& item)
{
  return ExecuteAction({"PlayMedia", *item, "resume"}, item);
}

bool CGUIBuiltinsUtils::ExecutePlayMediaNoResume(const std::shared_ptr<CFileItem>& item)
{
  return ExecuteAction({"PlayMedia", *item, "noresume"}, item);
}

bool CGUIBuiltinsUtils::ExecutePlayMediaAskResume(const std::shared_ptr<CFileItem>& item)
{
  return ExecuteAction({"PlayMedia", *item, ""}, item);
}

bool CGUIBuiltinsUtils::ExecuteQueueMedia(const std::shared_ptr<CFileItem>& item)
{
  return ExecuteAction({"QueueMedia", *item, ""}, item);
}
} // namespace KODI::UTILS::GUILIB

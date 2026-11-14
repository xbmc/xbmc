/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogOKHelper.h"

#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/Thread.h"

#include <memory>

namespace KODI
{
namespace MESSAGING
{
namespace HELPERS
{
namespace
{
int SendDialogOK(std::shared_ptr<DialogOKMessage> options)
{
  // the thread's owner may be stopping it while waiting for it, e.g. a player being closed
  if (CThread* thread = CThread::GetCurrentThread(); thread)
    return CServiceBroker::GetAppMessenger()->SendMsgAbortable(
        TMSG_GUI_DIALOG_OK, -1, -1, std::move(options), thread->GetStopEvent());
  return CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_DIALOG_OK, -1, -1, options.get());
}
} // namespace

bool ShowOKDialogText(CVariant heading, CVariant text)
{
  auto options = std::make_shared<DialogOKMessage>();
  options->heading = std::move(heading);
  options->text = std::move(text);

  return SendDialogOK(std::move(options)) > 0;
}

void UpdateOKDialogText(CVariant heading, CVariant text)
{
  DialogOKMessage options;
  options.heading = std::move(heading);
  options.text = std::move(text);
  options.show = false;

  CServiceBroker::GetAppMessenger()->SendMsg(TMSG_GUI_DIALOG_OK, -1, -1,
                                             static_cast<void*>(&options));
}

bool ShowOKDialogLines(CVariant heading, CVariant line0, CVariant line1, CVariant line2)
{
  auto options = std::make_shared<DialogOKMessage>();
  options->heading = std::move(heading);
  options->lines[0] = std::move(line0);
  options->lines[1] = std::move(line1);
  options->lines[2] = std::move(line2);

  return SendDialogOK(std::move(options)) > 0;
}

}
}
}

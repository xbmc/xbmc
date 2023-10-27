/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PVRGUIActionsParentalControl.h"

#include "ServiceBroker.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannel.h"
#include "settings/Settings.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <memory>
#include <string>

using namespace PVR;
using namespace KODI::MESSAGING;

CPVRGUIActionsParentalControl::CPVRGUIActionsParentalControl()
  : m_settings({CSettings::SETTING_PVRPARENTAL_PIN, CSettings::SETTING_PVRPARENTAL_ENABLED})
{
}

ParentalCheckResult CPVRGUIActionsParentalControl::CheckParentalLock(
    const std::shared_ptr<const CPVRChannel>& channel) const
{
  if (!CServiceBroker::GetPVRManager().IsParentalLocked(channel))
    return ParentalCheckResult::SUCCESS;

  ParentalCheckResult ret = CheckParentalPIN();

  if (ret == ParentalCheckResult::FAILED)
    CLog::LogF(LOGERROR, "Parental lock verification failed for channel '{}': wrong PIN entered.",
               channel->ChannelName());

  return ret;
}

ParentalCheckResult CPVRGUIActionsParentalControl::CheckParentalPIN() const
{
  if (!m_settings.GetBoolValue(CSettings::SETTING_PVRPARENTAL_ENABLED))
    return ParentalCheckResult::SUCCESS;

  std::string pinCode = m_settings.GetStringValue(CSettings::SETTING_PVRPARENTAL_PIN);
  if (pinCode.empty())
    return ParentalCheckResult::SUCCESS;

  InputVerificationResult ret = CGUIDialogNumeric::ShowAndVerifyInput(
      pinCode, g_localizeStrings.Get(19262), true); // "Parental control. Enter PIN:"

  if (ret == InputVerificationResult::SUCCESS)
  {
    CServiceBroker::GetPVRManager().RestartParentalTimer();
    return ParentalCheckResult::SUCCESS;
  }
  else if (ret == InputVerificationResult::FAILED)
  {
    HELPERS::ShowOKDialogText(CVariant{19264},
                              CVariant{19265}); // "Incorrect PIN", "The entered PIN was incorrect."
    return ParentalCheckResult::FAILED;
  }
  else
  {
    return ParentalCheckResult::CANCELED;
  }
}

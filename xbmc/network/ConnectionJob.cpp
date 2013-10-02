/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ConnectionJob.h"
#include "Application.h"
#include "utils/log.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogBusy.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "security/KeyringManager.h"
#include "settings/Settings.h"
#include "settings/AdvancedSettings.h"

CConnectionJob::CConnectionJob(CConnectionPtr connection, const CIPConfig &ipconfig, CKeyringManager *keyringManager)
{
  m_ipconfig = ipconfig;
  m_connection = connection;
  m_keyringManager = keyringManager;
}

bool CConnectionJob::DoWork()
{
  bool result;

  CGUIDialogBusy *busy_dialog = (CGUIDialogBusy*)g_windowManager.GetWindow(WINDOW_DIALOG_BUSY);
  busy_dialog->Show();

  // we need to shutdown network services before changing the connection.
  // The Network Manager's PumpNetworkEvents will take care of starting them back up.
  g_application.getNetwork().OnConnectionStateChange(NETWORK_CONNECTION_STATE_DISCONNECTED);
  result = m_connection->Connect((IPassphraseStorage*)this, m_ipconfig);
  if (!result)
  {
    // the connect failed, pop a failed dialog
    // and revert to the previous connection.
    // <string id="13297">Not connected. Check network settings.</string>
    CGUIDialogOK::ShowAndGetInput(0, 13297, 0, 0);
  }

  busy_dialog->Close();
  return result;
}

void CConnectionJob::InvalidatePassphrase(const std::string &uuid)
{
  m_keyringManager->EraseSecret("network", uuid);
  CSettings::Get().SetString("network.passphrase", "");
}

bool CConnectionJob::GetPassphrase(const std::string &uuid, std::string &passphrase)
{
  passphrase = CSettings::Get().GetString("network.passphrase");
  if (passphrase.size() > 0)
    return true;
  /*
  CVariant secret;
  if (m_keyringManager->FindSecret("network", uuid, secret) && secret.isString())
  {
    passphrase = secret.asString();
    return true;
  }
  */
  else
  {
    bool result;
    CStdString utf8;
    if (g_advancedSettings.m_showNetworkPassPhrase)
      result = CGUIKeyboardFactory::ShowAndGetInput(utf8, g_localizeStrings.Get(12340), false);
    else
      result = CGUIKeyboardFactory::ShowAndGetNewPassword(utf8);

    passphrase = utf8;
    StorePassphrase(uuid, passphrase);
    return result;
  }
}

void CConnectionJob::StorePassphrase(const std::string &uuid, const std::string &passphrase)
{
  m_keyringManager->StoreSecret("network", uuid, CVariant(passphrase));
  // hack until we get keyring storage working
  CSettings::Get().SetString("network.passphrase", passphrase.c_str());
}

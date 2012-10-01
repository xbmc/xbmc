/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "GUIOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "interfaces/Builtins.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "addons/AddonManager.h"
#include "settings/GUISettings.h"
#include "utils/Variant.h"

using namespace std;
using namespace JSONRPC;
using namespace ADDON;

JSONRPC_STATUS CGUIOperations::GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    CStdString propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CGUIOperations::ActivateWindow(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant params = parameterObject["parameters"];
  std::string cmd = "ActivateWindow(" + parameterObject["window"].asString();
  for (CVariant::iterator_array param = params.begin_array(); param != params.end_array(); param++)
  {
    if (param->isString() && !param->empty())
      cmd += "," + param->asString();
  }
  cmd += ")";
  CBuiltins::Execute(cmd);

  return ACK;
}

JSONRPC_STATUS CGUIOperations::ShowNotification(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  string image = parameterObject["image"].asString();
  string title = parameterObject["title"].asString();
  string message = parameterObject["message"].asString();
  unsigned int displaytime = (unsigned int)parameterObject["displaytime"].asUnsignedInteger();

  if (image.compare("info") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, title, message, displaytime);
  else if (image.compare("warning") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Warning, title, message, displaytime);
  else if (image.compare("error") == 0)
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, title, message, displaytime);
  else
    CGUIDialogKaiToast::QueueNotification(image, title, message, displaytime);

  return ACK;
}

JSONRPC_STATUS CGUIOperations::SetFullscreen(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ((parameterObject["fullscreen"].isString() &&
       parameterObject["fullscreen"].asString().compare("toggle") == 0) ||
      (parameterObject["fullscreen"].isBoolean() &&
       parameterObject["fullscreen"].asBoolean() != g_application.IsFullScreen()))
    CApplicationMessenger::Get().SendAction(CAction(ACTION_SHOW_GUI));
  else if (!parameterObject["fullscreen"].isBoolean() && !parameterObject["fullscreen"].isString())
    return InvalidParams;

  return GetPropertyValue("fullscreen", result);
}

JSONRPC_STATUS CGUIOperations::GetPropertyValue(const CStdString &property, CVariant &result)
{
  if (property.Equals("currentwindow"))
  {
    result["label"] = g_infoManager.GetLabel(g_infoManager.TranslateString("System.CurrentWindow"));
    result["id"] = g_windowManager.GetFocusedWindow();
  }
  else if (property.Equals("currentcontrol"))
    result["label"] = g_infoManager.GetLabel(g_infoManager.TranslateString("System.CurrentControl"));
  else if (property.Equals("skin"))
  {
    CStdString skinId = g_guiSettings.GetString("lookandfeel.skin");
    AddonPtr addon;
    CAddonMgr::Get().GetAddon(skinId, addon, ADDON_SKIN);

    result["id"] = skinId;
    if (addon.get())
      result["name"] = addon->Name();
  }
  else if (property.Equals("fullscreen"))
    result = g_application.IsFullScreen();
  else
    return InvalidParams;

  return OK;
}

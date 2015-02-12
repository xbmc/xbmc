/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include "GUIOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/Key.h"
#include "interfaces/Builtins.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "addons/AddonManager.h"
#include "settings/Settings.h"
#include "utils/Variant.h"
#include "guilib/StereoscopicsManager.h"
#include "windowing/WindowingFactory.h"

using namespace std;
using namespace JSONRPC;
using namespace ADDON;

JSONRPC_STATUS CGUIOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CGUIOperations::ActivateWindow(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CGUIOperations::ShowNotification(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CGUIOperations::SetFullscreen(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CGUIOperations::SetStereoscopicMode(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CAction action = CStereoscopicsManager::Get().ConvertActionCommandToAction("SetStereoMode", parameterObject["mode"].asString());
  if (action.GetID() != ACTION_NONE)
  {
    CApplicationMessenger::Get().SendAction(action);
    return ACK;
  }

  return InvalidParams;
}

JSONRPC_STATUS CGUIOperations::GetStereoscopicModes(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  for (int i = RENDER_STEREO_MODE_OFF; i < RENDER_STEREO_MODE_COUNT; i++)
  {
    RENDER_STEREO_MODE mode = (RENDER_STEREO_MODE) i;
    if (g_Windowing.SupportsStereo(mode))
      result["stereoscopicmodes"].push_back(GetStereoModeObjectFromGuiMode(mode));
  }

  return OK;
}

JSONRPC_STATUS CGUIOperations::GetPropertyValue(const std::string &property, CVariant &result)
{
  if (property == "currentwindow")
  {
    result["label"] = g_infoManager.GetLabel(g_infoManager.TranslateString("System.CurrentWindow"));
    result["id"] = g_windowManager.GetFocusedWindow();
  }
  else if (property == "currentcontrol")
    result["label"] = g_infoManager.GetLabel(g_infoManager.TranslateString("System.CurrentControl"));
  else if (property == "skin")
  {
    std::string skinId = CSettings::Get().GetString("lookandfeel.skin");
    AddonPtr addon;
    CAddonMgr::Get().GetAddon(skinId, addon, ADDON_SKIN);

    result["id"] = skinId;
    if (addon.get())
      result["name"] = addon->Name();
  }
  else if (property == "fullscreen")
    result = g_application.IsFullScreen();
  else if (property == "stereoscopicmode")
    result = GetStereoModeObjectFromGuiMode( CStereoscopicsManager::Get().GetStereoMode() );
  else
    return InvalidParams;

  return OK;
}

CVariant CGUIOperations::GetStereoModeObjectFromGuiMode(const RENDER_STEREO_MODE &mode)
{
  CVariant modeObj(CVariant::VariantTypeObject);
  modeObj["mode"] = CStereoscopicsManager::Get().ConvertGuiStereoModeToString(mode);
  modeObj["label"] = CStereoscopicsManager::Get().GetLabelForStereoMode(mode);
  return modeObj;
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "ApplicationOperations.h"
#include "InputOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/log.h"
#include "GUIInfoManager.h"
#include "system.h"
#include "GitRevision.h"

using namespace JSONRPC;

JSONRPC_STATUS CApplicationOperations::GetProperties(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
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

JSONRPC_STATUS CApplicationOperations::SetVolume(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  bool up = false;
  if (parameterObject["volume"].isInteger())
  {
    int oldVolume = (int)g_application.GetVolume();
    int volume = (int)parameterObject["volume"].asInteger();
  
    g_application.SetVolume((float)volume, true);

    up = oldVolume < volume;
  }
  else if (parameterObject["volume"].isString())
  {
    JSONRPC_STATUS ret;
    std::string direction = parameterObject["volume"].asString();
    if (direction.compare("increment") == 0)
    {
      ret = CInputOperations::SendAction(ACTION_VOLUME_UP, false, true);
      up = true;
    }
    else if (direction.compare("decrement") == 0)
    {
      ret = CInputOperations::SendAction(ACTION_VOLUME_DOWN, false, true);
      up = false;
    }
    else
      return InvalidParams;

    if (ret != ACK && ret != OK)
      return ret;
  }
  else
    return InvalidParams;

  CApplicationMessenger::Get().ShowVolumeBar(up);

  return GetPropertyValue("volume", result);
}

JSONRPC_STATUS CApplicationOperations::SetMute(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ((parameterObject["mute"].isString() && parameterObject["mute"].asString().compare("toggle") == 0) ||
      (parameterObject["mute"].isBoolean() && parameterObject["mute"].asBoolean() != g_application.IsMuted()))
    CApplicationMessenger::Get().SendAction(CAction(ACTION_MUTE));
  else if (!parameterObject["mute"].isBoolean() && !parameterObject["mute"].isString())
    return InvalidParams;

  return GetPropertyValue("muted", result);
}

JSONRPC_STATUS CApplicationOperations::Quit(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CApplicationMessenger::Get().Quit();
  return ACK;
}

JSONRPC_STATUS CApplicationOperations::GetPropertyValue(const CStdString &property, CVariant &result)
{
  if (property.Equals("volume"))
    result = (int)g_application.GetVolume();
  else if (property.Equals("muted"))
    result = g_application.IsMuted();
  else if (property.Equals("name"))
    result = "XBMC";
  else if (property.Equals("version"))
  {
    result = CVariant(CVariant::VariantTypeObject);
    result["major"] = VERSION_MAJOR;
    result["minor"] = VERSION_MINOR;
#ifdef GIT_REV
    result["revision"] = GIT_REV;
#endif
    CStdString tag(VERSION_TAG);
    if (tag.ToLower().Equals("-pre"))
      result["tag"] = "alpha";
    else if (tag.ToLower().Left(5).Equals("-beta"))
      result["tag"] = "beta";
    else if (tag.ToLower().Left(3).Equals("-rc"))
      result["tag"] = "releasecandidate";
    else if (tag.empty())
      result["tag"] = "stable";
    else
      result["tag"] = "prealpha";
  }
  else
    return InvalidParams;

  return OK;
}

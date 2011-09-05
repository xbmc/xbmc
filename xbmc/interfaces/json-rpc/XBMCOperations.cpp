/*
 *      Copyright (C) 2005-2010 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "XBMCOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/log.h"

using namespace JSONRPC;

JSON_STATUS CXBMCOperations::Play(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CFileItemList list;
  if (FillFileItemList(parameterObject, list) && list.Size() > 0)
  {
    g_application.getApplicationMessenger().MediaPlay(list);
    return ACK;
  }
  else
    return InvalidParams;
}

JSON_STATUS CXBMCOperations::StartSlideshow(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CStdString exec = "slideShow(";

  exec += parameterObject["directory"].asString();

  if (parameterObject["random"].asBoolean())
    exec += ", random";
  else
    exec += ", notrandom";

  if (parameterObject["recursive"].asBoolean())
    exec += ", recursive";

  exec += ")";
  ThreadMessage msg = { TMSG_EXECUTE_BUILT_IN, (DWORD)0, (DWORD)0, exec };
  g_application.getApplicationMessenger().SendMessage(msg);

  return ACK;
}

/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "InputOperations.h"
#include "Application.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CInputOperations::Left(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_MOVE_LEFT), WINDOW_INVALID, false);
  return ACK;
}

JSON_STATUS CInputOperations::Right(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_MOVE_RIGHT), WINDOW_INVALID, false);
  return ACK;
}

JSON_STATUS CInputOperations::Down(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_MOVE_DOWN), WINDOW_INVALID, false);
  return ACK;
}

JSON_STATUS CInputOperations::Up(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_MOVE_UP), WINDOW_INVALID, false);
  return ACK;
}

JSON_STATUS CInputOperations::Select(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_SELECT_ITEM), WINDOW_INVALID, false);
  return ACK;
}

JSON_STATUS CInputOperations::Back(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().SendAction(CAction(ACTION_PARENT_DIR), WINDOW_INVALID, false);
  return ACK;
}

JSON_STATUS CInputOperations::Home(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ActivateWindow(WINDOW_HOME, std::vector<CStdString>(), false);
  return ACK;
}

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
#include "guilib/GUIAudioManager.h"

using namespace JSONRPC;

//TODO the breakage of the screensaver should be refactored
//to one central super duper place for getting rid of
//1 million dupes
bool CInputOperations::handleScreenSaver()
{
  bool screenSaverBroken = false; //true if screensaver was active and we did reset him

  g_application.ResetScreenSaver();
  
  if(g_application.IsInScreenSaver())
  {
    g_application.WakeUpScreenSaverAndDPMS();
    screenSaverBroken = true;
  }
  return screenSaverBroken;
}

JSON_STATUS CInputOperations::sendAction(int actionID)
{
  if(!handleScreenSaver())
  {
    g_application.ResetSystemIdleTimer();
    g_audioManager.PlayActionSound(actionID);
    g_application.getApplicationMessenger().SendAction(CAction(actionID), WINDOW_INVALID, false);
  }
  return ACK;
}

JSON_STATUS CInputOperations::activateWindow(int windowID)
{
  if(!handleScreenSaver())
  {
    g_application.getApplicationMessenger().ActivateWindow(windowID, std::vector<CStdString>(), false);
  }
  return ACK;
}

JSON_STATUS CInputOperations::Left(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return sendAction(ACTION_MOVE_LEFT);
}

JSON_STATUS CInputOperations::Right(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return sendAction(ACTION_MOVE_RIGHT);  
}

JSON_STATUS CInputOperations::Down(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return sendAction(ACTION_MOVE_DOWN);  
}

JSON_STATUS CInputOperations::Up(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return sendAction(ACTION_MOVE_UP);  
}

JSON_STATUS CInputOperations::Select(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return sendAction(ACTION_SELECT_ITEM);  
}

JSON_STATUS CInputOperations::Back(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return sendAction(ACTION_NAV_BACK);  
}

JSON_STATUS CInputOperations::Home(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return activateWindow(WINDOW_HOME);
}

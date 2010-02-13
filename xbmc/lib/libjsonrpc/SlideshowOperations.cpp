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

#include "SlideshowOperations.h"
#include "Application.h"
#include "Key.h"

using namespace Json;
using namespace JSONRPC;


JSON_STATUS CSlideshowOperations::PlayPause(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_PAUSE);
}

JSON_STATUS CSlideshowOperations::Stop(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_STOP);
}

JSON_STATUS CSlideshowOperations::SkipPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_PREV_PICTURE);
}

JSON_STATUS CSlideshowOperations::SkipNext(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_NEXT_PICTURE);
}

JSON_STATUS CSlideshowOperations::MoveLeft(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_LEFT);
}

JSON_STATUS CSlideshowOperations::MoveRight(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_RIGHT);
}

JSON_STATUS CSlideshowOperations::MoveDown(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_DOWN);
}

JSON_STATUS CSlideshowOperations::MoveUp(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_UP);
}

JSON_STATUS CSlideshowOperations::ZoomOut(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_ZOOM_OUT);
}

JSON_STATUS CSlideshowOperations::ZoomIn(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_ZOOM_IN);
}

JSON_STATUS CSlideshowOperations::Zoom(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  if (!(parameterObject.isInt() || parameterObject.isNull()))
    return InvalidParams;

  int zoom = parameterObject.isInt() ? parameterObject.asInt() : 1;
  if (zoom > 10 || zoom <= 0)
    return InvalidParams;
 
  return SendAction(ACTION_ZOOM_LEVEL_NORMAL + (zoom - 1));
}

JSON_STATUS CSlideshowOperations::Rotate(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
  return SendAction(ACTION_ROTATE_PICTURE);
}

/*JSON_STATUS Move(const CStdString &method, ITransportLayer *transport, IClient *client, const Value& parameterObject, Value &result)
{
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
          CAction action;
          action.id = ACTION_ANALOG_MOVE;
          action.amount1=(float) atof(paras[0]);
          action.amount2=(float) atof(paras[1]);
          pSlideShow->OnAction(action);    
}*/

JSON_STATUS CSlideshowOperations::SendAction(int actionID)
{
  CAction action;
  action.actionId = actionID;
  g_application.getApplicationMessenger().SendAction(action, WINDOW_SLIDESHOW);

  return ACK;
}











   

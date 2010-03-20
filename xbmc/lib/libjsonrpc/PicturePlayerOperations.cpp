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

#include "PicturePlayerOperations.h"
#include "Application.h"
#include "Key.h"

using namespace Json;
using namespace JSONRPC;


JSON_STATUS CPicturePlayerOperations::PlayPause(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_PAUSE);
}

JSON_STATUS CPicturePlayerOperations::Stop(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_STOP);
}

JSON_STATUS CPicturePlayerOperations::SkipPrevious(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_PREV_PICTURE);
}

JSON_STATUS CPicturePlayerOperations::SkipNext(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_NEXT_PICTURE);
}

JSON_STATUS CPicturePlayerOperations::MoveLeft(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_LEFT);
}

JSON_STATUS CPicturePlayerOperations::MoveRight(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_RIGHT);
}

JSON_STATUS CPicturePlayerOperations::MoveDown(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_DOWN);
}

JSON_STATUS CPicturePlayerOperations::MoveUp(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_MOVE_UP);
}

JSON_STATUS CPicturePlayerOperations::ZoomOut(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_ZOOM_OUT);
}

JSON_STATUS CPicturePlayerOperations::ZoomIn(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_ZOOM_IN);
}

JSON_STATUS CPicturePlayerOperations::Zoom(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!(parameterObject.isInt() || parameterObject.isNull()))
    return InvalidParams;

  int zoom = parameterObject.isInt() ? parameterObject.asInt() : 1;
  if (zoom > 10 || zoom <= 0)
    return InvalidParams;
 
  return SendAction(ACTION_ZOOM_LEVEL_NORMAL + (zoom - 1));
}

JSON_STATUS CPicturePlayerOperations::Rotate(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  return SendAction(ACTION_ROTATE_PICTURE);
}

/*JSON_STATUS Move(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
      CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
      if (pSlideShow) {
          CAction action;
          action.id = ACTION_ANALOG_MOVE;
          action.GetAmount()=(float) atof(paras[0]);
          action.GetAmount(1)=(float) atof(paras[1]);
          pSlideShow->OnAction(action);    
}*/

JSON_STATUS CPicturePlayerOperations::SendAction(int actionID)
{
  g_application.getApplicationMessenger().SendAction(CAction(actionID), WINDOW_SLIDESHOW);

  return ACK;
}











   

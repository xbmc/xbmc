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

#ifndef WINDOW_EVENTS_H
#define WINDOW_EVENTS_H

#pragma once

#include "utils/Observer.h"
#include "XBMC_events.h"

typedef bool (* PHANDLE_EVENT_FUNC)(XBMC_Event& newEvent);

class IWinEvents : public Observer
{
  public:
    virtual       ~IWinEvents() {};
    virtual bool  MessagePump()   = 0;
    virtual void  MessagePush(XBMC_Event* ev) {};
    virtual void  Notify(const Observable &obs, const ObservableMessage msg) {};
};
class CWinEvents
{
  public:
    static void MessagePush(XBMC_Event* ev);
    static bool MessagePump();
};

#endif // WINDOW_EVENTS_H

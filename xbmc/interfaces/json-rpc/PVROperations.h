#pragma once
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

#include "utils/StdString.h"
#include "JSONRPC.h"

namespace JSONRPC
{
  class CPVROperations
  {
  public:
    static JSON_STATUS ChannelSwitch(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS ChannelUp(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS ChannelDown(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS RecordCurrentChannel(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS ScheduleRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS IsAvailable(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS IsScanningChannels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS IsRecording(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS ScanChannels(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
  };
}

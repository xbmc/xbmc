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
  class CInputOperations
  {
  public:
    static JSON_STATUS Left(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Right(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Down(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    static JSON_STATUS Up(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);


    static JSON_STATUS Select(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSON_STATUS Back(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);

    static JSON_STATUS Home(const CStdString &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result);
    
  private:
    static JSON_STATUS sendAction(int actionID);
    static JSON_STATUS activateWindow(int windowID);
    static bool        handleScreenSaver();
  };
}

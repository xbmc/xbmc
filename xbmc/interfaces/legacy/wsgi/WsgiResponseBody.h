#pragma once
/*
 *      Copyright (C) 2015 Team XBMC
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

#include "interfaces/legacy/AddonClass.h"

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    /**
     * Represents the write callable returned by the start_response callable passed to a WSGI handler.
     */
    class WsgiResponseBody : public AddonClass
    {
    public:
      WsgiResponseBody();
      virtual ~WsgiResponseBody();

      /**
      * Callable implemention to write data to the response.
      */
      void operator()(const String& data);

#ifndef SWIG
      String m_data;
#endif
    };    
  }
}



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
    /// \defgroup python_xbmcwsgi_WsgiResponseBody WsgiResponseBody
    /// \ingroup python_xbmcwsgi
    /// @{
    /// @brief **Represents the write callable returned by the start_response callable passed to a WSGI handler.**
    ///
    /// \python_class{ WsgiResponseBody() }
    ///
    ///-------------------------------------------------------------------------
    ///
    class WsgiResponseBody : public AddonClass
    {
    public:
      WsgiResponseBody();
      virtual ~WsgiResponseBody();

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcwsgi_WsgiInputStreamIterator
      /// \python_func{ operator(status, response_headers[, exc_info]) }
      ///------------------------------------------------------------------------
      ///
      /// Callable implemention to write data to the response.
      ///
      /// @param data            string data to write
      ///
      operator()(...);
#else
      void operator()(const String& data);
#endif

#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
      String m_data;
#endif
    };
  }
}

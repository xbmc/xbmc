/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
      ~WsgiResponseBody() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcwsgi_WsgiInputStreamIterator
      /// \python_func{ operator(status, response_headers[, exc_info]) }
      ///------------------------------------------------------------------------
      ///
      /// Callable implementation to write data to the response.
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

/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <vector>

#include "interfaces/legacy/AddonClass.h"

struct HTTPPythonRequest;

namespace XBMCAddon
{
  namespace xbmcwsgi
  {

    /// \defgroup python_xbmcwsgi_WsgiErrorStream WsgiErrorStream
    /// \ingroup python_xbmcwsgi
    /// @{
    /// @brief **Represents the wsgi.errors stream to write error messages.**
    ///
    /// \python_class{ WsgiErrorStream() }
    ///
    /// This implementation writes the error messages to the application's log
    /// file.
    ///
    ///-------------------------------------------------------------------------
    ///
    class WsgiErrorStream : public AddonClass
    {
    public:
      WsgiErrorStream();
      ~WsgiErrorStream() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcwsgi_WsgiErrorStream
      /// \python_func{ flush() }
      ///------------------------------------------------------------------------
      ///
      /// Since nothing is buffered this is a no-op.
      ///
      flush();
#else
      inline void flush() { }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcwsgi_WsgiErrorStream
      /// \python_func{ write(str) }
      ///------------------------------------------------------------------------
      ///
      /// Writes the given error message to the application's log file.
      ///
      /// A trailing `\n` is removed.
      ///
      /// @param str      A string to save in log file
      ///
      write(...);
#else
      void write(const String& str);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcwsgi_WsgiErrorStream
      /// \python_func{ writelines(seq) }
      ///------------------------------------------------------------------------
      ///
      /// Joins the given list of error messages (without any separator) into
      /// a single error message which is written to the application's log file.
      ///
      /// @param seq      A list of strings which will be logged.
      ///
      writelines(...);
#else
      void writelines(const std::vector<String>& seq);
#endif

#ifndef SWIG
      /**
       * Sets the given request.
       */
      void SetRequest(HTTPPythonRequest* request);

      HTTPPythonRequest* m_request;
#endif
    };
    /// @}
  }
}

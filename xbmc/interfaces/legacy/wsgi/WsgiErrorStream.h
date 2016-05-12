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
      virtual ~WsgiErrorStream();

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

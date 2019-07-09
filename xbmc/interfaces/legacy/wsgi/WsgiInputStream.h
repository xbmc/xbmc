/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/legacy/AddonClass.h"

#include <vector>

struct HTTPPythonRequest;

namespace XBMCAddon
{
  namespace xbmcwsgi
  {

    // Iterator for the wsgi.input stream.
    class WsgiInputStreamIterator : public AddonClass
    {
    public:
      WsgiInputStreamIterator();
      ~WsgiInputStreamIterator() override;

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcwsgi_WsgiInputStream
      /// \python_func{ read([size]) }
      ///------------------------------------------------------------------------
      ///
      /// Read a maximum of `<size>` bytes from the wsgi.input stream.
      ///
      /// @param size         [opt] bytes to read
      /// @return             Returns the readed string
      ///
      read(...);
#else
      String read(unsigned long size = 0) const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcwsgi_WsgiInputStream
      /// \python_func{ readline([size]) }
      ///------------------------------------------------------------------------
      ///
      /// Read a full line up to a maximum of `<size>` bytes from the wsgi.input
      /// stream.
      ///
      /// @param size         [opt] bytes to read
      /// @return             Returns the readed string line
      ///
      read(...);
#else
      String readline(unsigned long size = 0) const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      /// \ingroup python_xbmcwsgi_WsgiInputStream
      /// \python_func{ readlines([sizehint]) }
      ///------------------------------------------------------------------------
      ///
      /// Read multiple full lines up to at least `<sizehint>` bytes from the
      /// wsgi.input stream and return them as a list.
      ///
      /// @param sizehint      [opt] bytes to read
      /// @return              Returns a list readed string lines
      ///
      read(...);
#else
      std::vector<String> readlines(unsigned long sizehint = 0) const;
#endif

#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
      WsgiInputStreamIterator(const String& data, bool end = false);

      WsgiInputStreamIterator& operator++();
      bool operator==(const WsgiInputStreamIterator& rhs);
      bool operator!=(const WsgiInputStreamIterator& rhs);
      String& operator*();
      inline bool end() const { return m_remaining <= 0; }

    protected:
      String m_data;
      mutable unsigned long m_offset = 0;
      mutable unsigned long m_remaining = 0;

    private:
      String m_line;
#endif
    };

    /// \defgroup python_xbmcwsgi_WsgiInputStream WsgiInputStream
    /// \ingroup python_xbmcwsgi
    /// @{
    /// @brief **Represents the wsgi.input stream to access data from a HTTP request.**
    ///
    /// \python_class{ WsgiInputStream() }
    ///
    ///-------------------------------------------------------------------------
    ///
    class WsgiInputStream : public WsgiInputStreamIterator
    {
    public:
      WsgiInputStream();
      ~WsgiInputStream() override;

#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
      WsgiInputStreamIterator* begin();
      WsgiInputStreamIterator* end();

      /**
       * Sets the given request.
       */
      void SetRequest(HTTPPythonRequest* request);

      HTTPPythonRequest* m_request;
#endif
    };
  }
}

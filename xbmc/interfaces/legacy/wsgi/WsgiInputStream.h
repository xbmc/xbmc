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

    // Iterator for the wsgi.input stream.
    class WsgiInputStreamIterator : public AddonClass
    {
    public:
      WsgiInputStreamIterator();
      virtual ~WsgiInputStreamIterator();

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
      mutable unsigned long m_offset;
      mutable unsigned long m_remaining;

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
      virtual ~WsgiInputStream();

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

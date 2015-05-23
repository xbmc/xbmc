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
    /**
    * Iterator for the wsgi.input stream.
    */
    class WsgiInputStreamIterator : public AddonClass
    {
    public:
      WsgiInputStreamIterator();
      virtual ~WsgiInputStreamIterator();

      /**
      * Read a maximum of <size> bytes from the wsgi.input stream.
      */
      String read(unsigned long size = 0) const;

      /**
      * Read a full line up to a maximum of <size> bytes from the wsgi.input
      * stream.
      */
      String readline(unsigned long size = 0) const;

      /**
      * Read multiple full lines up to at least <sizehint> bytes from the
      * wsgi.input stream and return them as a list.
      */
      std::vector<String> readlines(unsigned long sizehint = 0) const;

#ifndef SWIG
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

    /**
     * Represents the wsgi.input stream to access data from a HTTP request.
     */
    class WsgiInputStream : public WsgiInputStreamIterator
    {
    public:
      WsgiInputStream();
      virtual ~WsgiInputStream();

#ifndef SWIG
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



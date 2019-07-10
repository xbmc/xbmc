/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WsgiInputStream.h"

#include "network/httprequesthandler/python/HTTPPythonRequest.h"
#include "utils/StringUtils.h"

namespace XBMCAddon
{
  namespace xbmcwsgi
  {
    WsgiInputStreamIterator::WsgiInputStreamIterator()
      : m_data(),
        m_line()
    { }

#ifndef SWIG
    WsgiInputStreamIterator::WsgiInputStreamIterator(const String& data, bool end /* = false */)
      : m_data(data),
        m_remaining(end ? 0 : data.size()),
        m_line()
    { }
#endif

    WsgiInputStreamIterator::~WsgiInputStreamIterator() = default;

    String WsgiInputStreamIterator::read(unsigned long size /* = 0 */) const
    {
      // make sure we don't try to read more data than we have
      if (size <= 0 || size > m_remaining)
        size = m_remaining;

      // remember the current read offset
      size_t offset = static_cast<size_t>(m_offset);

      // adjust the read offset and the remaining data length
      m_offset += size;
      m_remaining -= size;

      // return the data being requested
      return m_data.substr(offset, size);
    }

    String WsgiInputStreamIterator::readline(unsigned long size /* = 0 */) const
    {
      // make sure we don't try to read more data than we have
      if (size <= 0 || size > m_remaining)
        size = m_remaining;

      size_t offset = static_cast<size_t>(m_offset);
      size_t pos = m_data.find('\n', offset);

      // make sure pos has a valid value and includes the \n character
      if (pos == std::string::npos)
        pos = m_data.size();
      else
        pos += 1;

      if (pos - offset < size)
        size = pos - offset;

      // read the next line
      String line = read(size);

      // remove any trailing \r\n
      StringUtils::TrimRight(line, "\r\n");

      return line;
    }

    std::vector<String> WsgiInputStreamIterator::readlines(unsigned long sizehint /* = 0 */) const
    {
      std::vector<String> lines;

      // make sure we don't try to read more data than we have
      if (sizehint <= 0 || sizehint > m_remaining)
        sizehint = m_remaining;

      do
      {
        // read a full line
        String line = readline();

        // adjust the sizehint by the number of bytes just read
        sizehint -= line.length();

        // add it to the list of read lines
        lines.push_back(line);
      } while (sizehint > 0);

      return lines;
    }

#ifndef SWIG
    WsgiInputStreamIterator& WsgiInputStreamIterator::operator++()
    {
      m_line.clear();

      if (!end())
      {
        // read the next line
        m_line = readline();
      }

      return *this;
    }

    bool WsgiInputStreamIterator::operator==(const WsgiInputStreamIterator& rhs)
    {
      return m_data == rhs.m_data &&
             m_offset == rhs.m_offset &&
             m_remaining == rhs.m_remaining;
    }

    bool WsgiInputStreamIterator::operator!=(const WsgiInputStreamIterator& rhs)
    {
      return !(*this == rhs);
    }

    String& WsgiInputStreamIterator::operator*()
    {
      return m_line;
    }
#endif

    WsgiInputStream::WsgiInputStream()
      : m_request(NULL)
    { }

    WsgiInputStream::~WsgiInputStream()
    {
      m_request = NULL;
    }

#ifndef SWIG
    WsgiInputStreamIterator* WsgiInputStream::begin()
    {
      return new WsgiInputStreamIterator(m_data, false);
    }

    WsgiInputStreamIterator* WsgiInputStream::end()
    {
      return new WsgiInputStreamIterator(m_data, true);
    }

    void WsgiInputStream::SetRequest(HTTPPythonRequest* request)
    {
      if (m_request != NULL)
        return;

      m_request = request;

      // set the remaining bytes to be read
      m_data = m_request->requestContent;
      m_offset = 0;
      m_remaining = m_data.size();
    }
#endif
  }
}

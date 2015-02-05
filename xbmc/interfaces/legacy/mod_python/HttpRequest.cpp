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

#include <algorithm>

#include "HttpRequest.h"
#include "URL.h"
#include "interfaces/legacy/AddonUtils.h"
#include "utils/log.h"
#include "utils/URIUtils.h"

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    HttpRequest::HttpRequest()
      : assbackwards(true),
        header_only(false),
        protocol("HTTP/0.9"),
        proto_num(9),
        request_time(0),
        method("GET"),
        mtime(0),
        clength(0),
        remaining(0),
        read_length(0),
        headers_out(new Table),
        err_headers_out(new Table),
        m_request(NULL),
        m_forcedLength(false)
    { }

    HttpRequest::~HttpRequest()
    {
      m_request = NULL;

      delete headers_out;
      delete err_headers_out;
    }

    void HttpRequest::allow_methods(const std::vector<std::string>& methods, bool reset /* = false */) throw(InvalidArgumentException)
    {
      if (reset)
        allowed_methods.clear();

      for (std::vector<std::string>::const_iterator method = methods.begin(); method != methods.end(); ++method)
      {
        if (*method != HTTP_METHOD_GET && *method != HTTP_METHOD_HEAD && *method != HTTP_METHOD_POST)
          throw InvalidArgumentException("methods");

        // only add the method if it isn't already part of the list
        if (std::find(allowed_methods.begin(), allowed_methods.end(), *method) == allowed_methods.end())
          allowed_methods.push_back(*method);
      }
    }

    String HttpRequest::construct_url(const String& uri) const
    {
      CURL url;
      if (is_https())
        url.SetProtocol("https");
      else
        url.SetProtocol("http");

      if (hostname.empty())
        url.SetHostName("localhost");
      else
        url.SetHostName(hostname);

      if (m_request->port != 80)
        url.SetPort(m_request->port);

      url.SetFileName(uri);

      return url.Get();
    }
    
    void HttpRequest::log_error(const char* msg, int level /* = lLOGERROR */)
    {
      if (msg == NULL)
        return;

      if (level < LOGDEBUG || level > LOGNONE)
        level = LOGNOTICE;

      CLog::Log(level, "%s", msg);
    }

    String HttpRequest::read(long len /* = -1 */)
    {
      // make sure we don't try to read more data than we have
      if (len < 0 || len > remaining)
        len = remaining;

      if (len == 0)
        return "";

      // remember the current read offset
      size_t offset = static_cast<size_t>(read_length);

      // adjust the read offset and the remaining data length
      read_length += len;
      remaining -= len;

      // return the data being requested
      return m_request->requestContent.substr(offset, len);
    }

    String HttpRequest::readline(long len /* = -1 */)
    {
      // make sure we don't try to read more data than we have
      if (len < 0 || len > remaining)
        len = remaining;

      if (len == 0)
        return "";

      size_t offset = static_cast<size_t>(read_length);
      size_t pos = m_request->requestContent.find('\n', offset);

      // make sure pos has a valid value and includes the \n character
      if (pos == std::string::npos)
        pos = m_request->requestContent.size();
      else
        pos += 1;

      if (pos - offset < static_cast<size_t>(len))
        len = pos - offset;

      // read the next line
      String line = read(len);

      // remove any trailing \r\n
      StringUtils::TrimRight(line, "\r\n");

      return line;
    }

    std::vector<String> HttpRequest::readlines(long sizehint /* = -1 */)
    {
      std::vector<String> lines;

      // make sure we don't try to read more data than we have
      if (sizehint < 0 || sizehint > remaining)
        sizehint = remaining;

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

    void HttpRequest::set_last_modified()
    {
      if (mtime <= 0)
        return;

      CDateTime lastModified(static_cast<time_t>(mtime));
      if (!lastModified.IsValid())
        return;

      m_request->lastModifiedTime = lastModified;
    }

    void HttpRequest::update_mtime(long updated_mtime)
    {
      if (updated_mtime < 0 || updated_mtime <= mtime)
        return;

      mtime = updated_mtime;
    }

    void HttpRequest::write(String text)
    {
      if (m_request == NULL)
        return;

      m_request->responseData.append(text);

      // update the value of clength
      if (!m_forcedLength)
        clength = m_request->responseData.size();
    }

    void HttpRequest::set_content_length(long len) throw(InvalidArgumentException)
    {
      if (len < 0)
        throw InvalidArgumentException("len");

      clength = len;
      m_forcedLength = true;
    }

#ifndef SWIG
    void HttpRequest::SetRequest(HTTPPythonRequest* request)
    {
      if (m_request != NULL)
        return;

      m_request = request;

      CURL url(m_request->url);

      // set method
      switch (m_request->method)
      {
      case HEAD:
        method = "HEAD";
        break;

      case POST:
        method = "POST";
        break;

      case GET:
      default:
        method = "GET";
        break;
      }

      // set the_request
      the_request = StringUtils::Format("%s %s %s", m_request->version.c_str(), method.c_str(), m_request->url.c_str());

      // set header_only
      header_only = m_request->method == HEAD;

      // set protocol, proto_num and assbackwards
      if (!m_request->version.empty())
      {
        protocol = m_request->version;

        long major = 0;
        long minor = 0;

        size_t posSlash = protocol.find("/");
        size_t posDot = protocol.find(".");
        if (posSlash != std::string::npos && posDot != std::string::npos)
        {
          major = strtol(protocol.substr(posSlash + 1, posDot - posSlash - 1).c_str(), NULL, 0);
          minor = strtol(protocol.substr(posDot + 1, protocol.size() - posDot - 1).c_str(), NULL, 0);

          proto_num = static_cast<int>(major * 1000 + minor);
        }

        assbackwards = proto_num == 9;
      }

      // set hostname
      hostname = m_request->hostname;

      // set request_time
      if (m_request->requestTime.IsValid())
      {
        time_t tm;
        m_request->requestTime.GetAsTime(tm);
        request_time = tm;
      }

      // set status
      status = m_request->responseStatus;

      // set mtime
      if (m_request->lastModifiedTime.IsValid())
        m_request->lastModifiedTime.GetAsTime(mtime);

      // set remaining
      remaining = static_cast<long>(m_request->requestContent.size());

      // set read_length
      read_length = 0;

      // set range
      std::multimap<String, String>::const_iterator headerValue = m_request->headerValues.find(MHD_HTTP_HEADER_RANGE);
      if (headerValue != m_request->headerValues.end())
        range = headerValue->second;

      // set headers_in
      headers_in.insert(m_request->headerValues.begin(), m_request->headerValues.end());

      // set content_encoding
      headerValue = m_request->headerValues.find(MHD_HTTP_HEADER_CONTENT_ENCODING);
      if (headerValue != m_request->headerValues.end())
        content_encoding = headerValue->second;

      // set unparsed_uri
      unparsed_uri = m_request->url;

      // set uri
      uri = m_request->path;

      // set filename
      filename = m_request->file;

      // set canonical_filename
      canonical_filename = filename;

      // set args
      args = url.GetOptions();

      // set get and post
      get.insert(m_request->getValues.begin(), m_request->getValues.end());
      post.insert(m_request->postValues.begin(), m_request->postValues.end());
    }

    HTTPPythonRequest* HttpRequest::Finalize()
    {
      if (m_request == NULL)
        return NULL;

      m_request->responseStatus = status;

      if (!content_type.empty())
        m_request->responseContentType = content_type;

      m_request->responseLength = clength;
      m_request->responseHeaders.insert(headers_out->m_map.begin(), headers_out->m_map.end());
      m_request->responseHeadersError.insert(err_headers_out->m_map.begin(), err_headers_out->m_map.end());

      // if the response status is either "Method Not Allowed" or "Not Implemented" and the allowed_methods list is not empty
      // add the "Allow" HTTP header
      if (m_request->responseStatus == MHD_HTTP_METHOD_NOT_ALLOWED || m_request->responseStatus == MHD_HTTP_NOT_IMPLEMENTED)
      {
        std::string allowed = StringUtils::Join(allowed_methods, ", ");
        m_request->responseHeadersError.insert(std::make_pair(MHD_HTTP_HEADER_ALLOW, allowed));
      }

      return m_request;
    }
#endif
  }
}

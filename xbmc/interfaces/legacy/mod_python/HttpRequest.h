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

#include "commons/Exception.h"
#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/Dictionary.h"
#include "interfaces/legacy/swighelper.h"
#include "interfaces/legacy/mod_python/Table.h"
#include "network/httprequesthandler/python/HTTPPythonRequest.h"

#ifndef SWIG
#define HTTP_METHOD_GET   "GET"
#define HTTP_METHOD_HEAD  "HEAD"
#define HTTP_METHOD_POST  "POST"
#endif

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    const int lLOGERROR = LOGERROR;

    /**
     * HttpRequest class representing a HTTP request.\n
     * \n
     * You can't create this object, it is automatically created and available as the global "req" object.
     */
    class HttpRequest : public AddonClass
    {
    public:
      HttpRequest();
      virtual ~HttpRequest();

      // methods matching Apache's mod_python
      /**
       * Adds methods to the HttpRequest.allowed_methods() list. This list will be passed in "Allow: header
       * if HTTP_METHOD_NOT_ALLOWED or HTTP_NOT_IMPLEMENTED is returned to the client. Note that there's no
       * additional logic to restrict the methods, this list is only used to construct the header.
       * The actual method-restricting logic has to be provided in the handler code.\n
       * \n
       * methods is a sequence of strings. If reset is 1, then the list of methods is first cleared.
       */
      void allow_methods(const std::vector<std::string>& methods, bool reset = false) throw(InvalidArgumentException);

      /**
       * This function returns a fully qualified URI string from the path specified by uri, using the information
       * stored in the request to determine the scheme, server name and port. The port number is not included in
       * the string if it is the same as the default port 80.\n
       * \n
       * For example, imagine that the current request is directed to the virtual server www.kodi.tv at port 80.$
       * Then supplying '/index.html' will yield the string 'http://www.kodi.tv/index.html'.
       */
      String construct_url(const String& uri) const;

      /**
       * is_https() -- Checks if the request has been made using HTTPS.\n
       */
      bool is_https() const { return false; }

      /**
       * log_error(msg[, level=LOGERROR]) -- Logs the given message with the given level.\n
       *      msg            : string - text to output.\n
       *      level          : [opt] integer - log level to ouput at. (default=LOGERROR)\n
       */
      void log_error(const char* msg, int level = lLOGERROR);

      /**
       * Reads at most len bytes directly from the client, returning a string with the data read.
       * If the len argument is negative or omitted, reads all data given by the client.\n
       * \n
       * This function relies on the client providing the Content-Length header. Absence of the
       * Content-Length header will be treated as if Content-length: 0 was supplied.\n
       * \n
       * Incorrect Content-Length may cause the function to try to read more data than available,
       * which will make the function block until a timeout is reached.
       */
      String read(long len = -1);

      /**
       * Like HttpRequest.read() but reads until end of line.\n
       * \n
       * NOTE: In accordance with the HTTP specification, most clients will be terminating lines
       * with '\r\n' rather than simply '\n'.
       */
      String readline(long len = -1);

      /**
       * Reads all lines using HttpRequest.readline() and returns a list of the lines read.
       * If the optional sizehint parameter is given in, the method will read at least sizehint
       * bytes of data, up to the completion of the line in which the sizehint bytes limit is reached.
       */
      std::vector<String> readlines(long sizehint = -1);

      /**
       * Sets the outgoing Last-Modified header based on value of mtime attribute.
       */
      void set_last_modified();

      /**
       * If updated_mtime is later than the value in the mtime attribute, sets the attribute to the new value.
       */
      void update_mtime(long updated_mtime);

      /**
       * Writes string directly to the client, then flushes the buffer, unless flush is 0. Unicode strings are encoded using utf-8 encoding.
       */
      void write(String text);

      /**
       * Sets the value of HttpRequest.clength and the Content-Length header to len.
       */
      void set_content_length(long len) throw(InvalidArgumentException);

      // members matching Apache's mod_python
      /**
       * String containing the first line of the request. (Read-Only)
       */
      SWIG_IMMUTABLE(String the_request);
      /**
       * Indicates an HTTP/0.9 "simple" request. This means that the response will contain no headers, only the body.
       */
      SWIG_IMMUTABLE(bool assbackwards);
      /**
       * A boolean value indicating HEAD request, as opposed to GET or POST. (Read-Only)
       */
      SWIG_IMMUTABLE(bool header_only);
      /**
       * Protocol, as given by the client, or 'HTTP/0.9'. (Read-Only)
       */
      SWIG_IMMUTABLE(String protocol);
      /**
       * Integer. Number version of protocol; 1.1 = 1001 (Read-Only)
       */
      SWIG_IMMUTABLE(int proto_num);
      /**
       * String. Host, as set by full URI or Host: header. (Read-Only)
       */
      SWIG_IMMUTABLE(String hostname);
      /**
       * A long integer. When request started. (Read-Only)
       */
      SWIG_IMMUTABLE(long request_time);
      /**
       * Status. One of xbmcmod_python.HTTP_* values.
       */
      int status;
      /**
       * A string containing the method - 'GET', 'HEAD', 'POST'. (Read-Only)
       */
      SWIG_IMMUTABLE(String method);
      /**
       * List of allowed methods. Used in relation with METHOD_NOT_ALLOWED. This member can be modified via HttpRequest.allow_methods() described in section Request Methods. (Read-Only)
       */
      SWIG_IMMUTABLE(std::vector<std::string> allowed_methods);
      /**
       * Long integer. Time the resource was last modified. (Read-Only)
       */
      SWIG_IMMUTABLE(long mtime);
      /**
       * String. The Range: header. (Read-Only)
       */
      SWIG_IMMUTABLE(String range);
      /**
       * Long integer. The "real" content length. (Read-Only)
       */
      SWIG_IMMUTABLE(long clength);
      /**
       * Long integer. Bytes left to read. (Only makes sense inside a read operation.) (Read-Only)
       */
      SWIG_IMMUTABLE(long remaining);
      /**
      * Long integer. Number of bytes read. (Read-Only)
      */
      SWIG_IMMUTABLE(long read_length);
      /**
       * A multi dict object (dict containing lists) containing headers sent by the client. (Read-Only)
       */
      SWIG_IMMUTABLE(XBMCAddon::MultiDictionary<String> headers_in);
      /**
       * Table object representing the headers to be sent to the client.
       */
      Table* headers_out;
      /**
       * These headers get send with the error response, instead of headers_out.
       */
      Table* err_headers_out;
      /**
       * String. The content type.
       */
      String content_type;
      /**
       * String. Content encoding. (Read-Only)
       */
      SWIG_IMMUTABLE(String content_encoding);
      /**
       * The URI without any parsing performed. (Read-Only)
       */
      SWIG_IMMUTABLE(String unparsed_uri);
      /**
       * The path portion of the URI. (Read-Only)
       */
      SWIG_IMMUTABLE(String uri);
      /**
       * String. File name being requested. (Read-Only)
       */
      SWIG_IMMUTABLE(String filename);
      /**
       * String. The true filename (filename is canonicalized if they don't match). (Read-Only)
       */
      SWIG_IMMUTABLE(String canonical_filename);
      /**
       * String. Query arguments. (Read-Only)
       */
      SWIG_IMMUTABLE(String args);

      // custom members
      /**
       * Dictionary. The GET values.
       */
      XBMCAddon::Dictionary<String> get;
      /**
       * Dictionary. The POST values.
       */
      SWIG_IMMUTABLE(XBMCAddon::Dictionary<String> post);

#ifndef SWIG
      /**
       * Sets the given request and extracts its information.
       */
      void SetRequest(HTTPPythonRequest* request);

      /**
       * Fills any data written into the member variables into the request and returns it.
       */
      HTTPPythonRequest* Finalize();

      HTTPPythonRequest* m_request;

      // whether clength has been manually set or not
      bool m_forcedLength;
#endif
    };    
  }
}



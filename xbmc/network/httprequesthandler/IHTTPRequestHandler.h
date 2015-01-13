#pragma once
/*
 *      Copyright (C) 2011-2013 Team XBMC
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

#include <string>
#include <map>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <microhttpd.h>

#include "XBDateTime.h"

class CWebServer;

enum HTTPMethod
{
  UNKNOWN,
  POST,
  GET,
  HEAD
};

enum HTTPResponseType
{
  HTTPNone,
  // creates and returns a HTTP error
  HTTPError,
  // creates and returns a HTTP redirect response
  HTTPRedirect,
  // creates a HTTP response with the content from a file
  HTTPFileDownload,
  // creates a HTTP response from a buffer without copying or freeing the buffer
  HTTPMemoryDownloadNoFreeNoCopy,
  // creates a HTTP response from a buffer by copying but not freeing the buffer
  HTTPMemoryDownloadNoFreeCopy,
  // creates a HTTP response from a buffer without copying followed by freeing the buffer
  // the buffer must have been malloc'ed and not new'ed
  HTTPMemoryDownloadFreeNoCopy,
  // creates a HTTP response from a buffer by copying followed by freeing the buffer
  // the buffer must have been malloc'ed and not new'ed
  HTTPMemoryDownloadFreeCopy
};

typedef struct HTTPRequest
{
  struct MHD_Connection *connection;
  std::string url;
  HTTPMethod method;
  std::string version;
  CWebServer *webserver;
} HTTPRequest;

class IHTTPRequestHandler
{
public:
  virtual ~IHTTPRequestHandler() { }

  /*!
   * \brief Creates a new HTTP request handler for the given request.
   *
   * \details This call is responsible for doing some preparation work like -
   * depending on the supported features - determining whether the requested
   * entity supports ranges, whether it can be cached and what it's last
   * modified date is.
   *
   * \param request HTTP request to be handled
   */
  virtual IHTTPRequestHandler* Create(const HTTPRequest &request) = 0;

  /*!
   * \brief Returns the priority of the HTTP request handler.
   *
   * \details The higher the priority the more important is the HTTP request
   * handler.
   */
  virtual int GetPriority() const { return 0; }

  /*!
  * \brief Checks if the HTTP request handler can handle the given request.
  *
  * \param request HTTP request to be handled
  * \return True if the given HTTP request can be handled otherwise false.
  */
  virtual bool CanHandleRequest(const HTTPRequest &request) = 0;

  /*!
   * \brief Returns the HTTP request handled by the HTTP request handler.
   */
  const HTTPRequest& GetRequest() { return m_request; }

  /*!
   * \brief Handles the HTTP request.
   *
   * \return MHD_NO if a severe error has occurred otherwise MHD_YES.
   */
  virtual int HandleRequest() = 0;

  /*!
  * \brief Returns the type of the response.
  */
  HTTPResponseType GetResponseType() const { return m_responseType; }

  /*!
  * \brief Returns the HTTP status of the response.
  */
  int GetResponseCode() const { return m_responseCode; }

  /*!
  * \brief Returns the HTTP response's header field-value pairs.
  */
  const std::multimap<std::string, std::string>& GetResponseHeaderFields() const { return m_responseHeaderFields; };
 
  /*!
   * \brief Returns the raw data of the response.
   *
   * \details This is only used if the response type is one of the HTTPMemoryDownload types.
   */
  virtual void* GetResponseData() const { return NULL; };
  /*!
  * \brief Returns the length of the raw data of the response.
  *
  * \details This is only used if the response type is one of the HTTPMemoryDownload types.
  */
  virtual size_t GetResponseDataLength() const { return 0; }

  /*!
  * \brief Returns the URL to which the request should be redirected.
  *
  * \details This is only used if the response type is HTTPRedirect.
  */
  virtual std::string GetRedirectUrl() const { return ""; }

  /*!
  * \brief Returns the path to the file that should be returned as the response.
  *
  * \details This is only used if the response type is HTTPFileDownload.
  */
  virtual std::string GetResponseFile() const { return ""; }

  /*!
   * \brief Adds the given key-value pair extracted from the HTTP POST data.
   *
   * \param key Key of the HTTP POST field
   * \param value Value of the HTTP POST field
   */
  void AddPostField(const std::string &key, const std::string &value);
#if (MHD_VERSION >= 0x00040001)
  /*!
  * \brief Adds the given raw HTTP POST data.
  *
  * \param data Raw HTTP POST data
  * \param size Size of the raw HTTP POST data
  */
  bool AddPostData(const char *data, size_t size);
#else
  bool AddPostData(const char *data, unsigned int size);
#endif

protected:
  IHTTPRequestHandler() { }
  explicit IHTTPRequestHandler(const HTTPRequest &request);

#if (MHD_VERSION >= 0x00040001)
  virtual bool appendPostData(const char *data, size_t size)
#else
  virtual bool appendPostData(const char *data, unsigned int size)
#endif
  { return true; }

  HTTPRequest m_request;

  int m_responseCode;
  HTTPResponseType m_responseType;
  std::multimap<std::string, std::string> m_responseHeaderFields;

  std::map<std::string, std::string> m_postFields;
};

/*
 *  Copyright (C) 2011-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace HTTP
{
  enum Version
  {
    Version1_0,
    Version1_1
  };

  enum Method
  {
    Get,
    Head,
    POST,
    PUT,
    Delete,
    Trace,
    Connect
  };

  enum StatusCode
  {
    // Information 1xx
    Continue                      = 100,
    SwitchingProtocols            = 101,
    Processing                    = 102,
    ConnectionTimedOut            = 103,

    // Success 2xx
    OK                            = 200,
    Created                       = 201,
    Accepted                      = 202,
    NonAuthoritativeInformation   = 203,
    NoContent                     = 204,
    ResetContent                  = 205,
    PartialContent                = 206,
    MultiStatus                   = 207,

    // Redirects 3xx
    MultipleChoices               = 300,
    MovedPermanently              = 301,
    Found                         = 302,
    SeeOther                      = 303,
    NotModified                   = 304,
    UseProxy                      = 305,
    //SwitchProxy                 = 306,
    TemporaryRedirect             = 307,

    // Client errors 4xx
    BadRequest                    = 400,
    Unauthorized                  = 401,
    PaymentRequired               = 402,
    Forbidden                     = 403,
    NotFound                      = 404,
    MethodNotAllowed              = 405,
    NotAcceptable                 = 406,
    ProxyAuthenticationRequired   = 407,
    RequestTimeout                = 408,
    Conflict                      = 409,
    Gone                          = 410,
    LengthRequired                = 411,
    PreconditionFailed            = 412,
    RequestEntityTooLarge         = 413,
    RequestURITooLong             = 414,
    UnsupportedMediaType          = 415,
    RequestedRangeNotSatisfiable  = 416,
    ExpectationFailed             = 417,
    ImATeapot                     = 418,
    TooManyConnections            = 421,
    UnprocessableEntity           = 422,
    Locked                        = 423,
    FailedDependency              = 424,
    UnorderedCollection           = 425,
    UpgradeRequired               = 426,

    // Server errors 5xx
    InternalServerError           = 500,
    NotImplemented                = 501,
    BadGateway                    = 502,
    ServiceUnavailable            = 503,
    GatewayTimeout                = 504,
    HTTPVersionNotSupported       = 505,
    VariantAlsoNegotiates         = 506,
    InsufficientStorage           = 507,
    BandwidthLimitExceeded        = 509,
    NotExtended                   = 510
  };
}

class CHttpResponse
{
public:
  CHttpResponse(HTTP::Method method, HTTP::StatusCode status, HTTP::Version version = HTTP::Version1_1);

  void AddHeader(const std::string &field, const std::string &value);
  void SetContent(const char* data, unsigned int length);

  std::string Create();

private:
  HTTP::Method m_method;
  HTTP::StatusCode m_status;
  HTTP::Version m_version;
  std::vector< std::pair<std::string, std::string> > m_headers;
  const char* m_content;
  unsigned int m_contentLength;
  std::string m_buffer;

  static std::map<HTTP::StatusCode, std::string> m_statusCodeText;
  static std::map<HTTP::StatusCode, std::string> createStatusCodes();
};

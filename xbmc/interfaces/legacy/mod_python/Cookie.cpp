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

#include <limits>

#include "Cookie.h"
#include "XBDateTime.h"
#include "utils/StringUtils.h"

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    Cookie::Cookie(String name, String value) throw(InvalidArgumentException)
      : name(name),
        value(value),
        version(1),
        secure(false),
        max_age(0),
        discard(false),
        httponly(false),
        max_age_set(false),
        max_age_default(max_age)
    {
      if (name.empty())
        throw InvalidArgumentException("Cookie cannot have an empty name");
    }

    Cookie::~Cookie()
    { }

    XBMCAddon::Dictionary<Cookie*> Cookie::parse(String data)
    {
      return parseCookies(data);
    }

    void Cookie::add_cookie(HttpRequest *request, Cookie *cookie) throw(NullPointerException)
    {
      if (request == NULL)
        throw NullPointerException("request");

      if (cookie == NULL)
        throw NullPointerException("cookie");

      request->m_request->responseHeaders.insert(std::make_pair(MHD_HTTP_HEADER_SET_COOKIE, cookie->toString()));
    }

    XBMCAddon::Dictionary<Cookie*> Cookie::get_cookies(HttpRequest *request) throw(NullPointerException)
    {
      if (request == NULL)
        throw NullPointerException("request");

      XBMCAddon::Dictionary<Cookie*> cookies;

      std::multimap<std::string, std::string>::const_iterator cookie = request->m_request->headerValues.find(MHD_HTTP_HEADER_COOKIE);
      if (cookie == request->m_request->headerValues.end())
        return XBMCAddon::Dictionary<Cookie*>();

      return parseCookies(cookie->second);
    }

    Cookie* Cookie::get_cookie(HttpRequest *request, String name) throw(NullPointerException)
    {
      if (request == NULL)
        throw NullPointerException("request");

      if (name.empty())
        return NULL;

      XBMCAddon::Dictionary<Cookie*> cookies = get_cookies(request);
      XBMCAddon::Dictionary<Cookie*>::iterator cookie = cookies.find(name);
      if (cookie == cookies.end())
        return NULL;

      return cookie->second;
    }
    
    std::string Cookie::toString()
    {
      // update the value of "expires" if max_age has been set/changed
      if (max_age_set || max_age != max_age_default)
        updateExpires(max_age);

      std::string str = name + "=" + value + "; " +
        StringUtils::Format("Version=%d", version);

      if (!path.empty())
        str += "; Path=" + path;
      if (!domain.empty())
        str += "; Domain=" + domain;
      if (secure)
        str += "; Secure";
      if (!comment.empty())
        str += "; Comment=\"" + comment + "\"";
      if (max_age_set)
      {
        if (!expires.empty())
          str += "; Expires=" + expires;
        str += StringUtils::Format("; Max-Age=%ld", max_age);
      }
      if (!commentURL.empty())
        str += "; CommentURL=\"" + commentURL + "\"";
      if (discard)
        str += "; Discard";
      if (!port.empty())
        str += "; Port=\"" + port + "\"";
      if (httponly)
        str += "; HttpOnly";

      return str;
    }

    XBMCAddon::Dictionary<Cookie*> Cookie::parseCookies(String data)
    {
      XBMCAddon::Dictionary<Cookie*> cookies;
      Cookie* cookie = NULL;
      std::vector<std::string> cookieParts = StringUtils::Split(data, ";");
      for (std::vector<std::string>::const_iterator it = cookieParts.begin(); it != cookieParts.end(); ++it)
      {
        std::string part = *it;
        StringUtils::Trim(part);

        if (part.empty())
          continue;

        std::string key = part;
        std::string value;
        size_t pos = part.find("=");
        if (pos != std::string::npos)
        {
          key = part.substr(0, pos);
          value = part.substr(pos + 1);

          StringUtils::Trim(key);
          StringUtils::Trim(value);
        }

        if (key.empty())
          continue;

        // check if this is a new cookie
        if (cookie == NULL)
        {
          cookie = new Cookie(key, value);
          continue;
        }

        // check if we found a string attribute
        if (StringUtils::EqualsNoCase(key, "Path") ||
            StringUtils::EqualsNoCase(key, "Domain") ||
            StringUtils::EqualsNoCase(key, "Comment") ||
            StringUtils::EqualsNoCase(key, "Expires") ||
            StringUtils::EqualsNoCase(key, "CommentURL") ||
            StringUtils::EqualsNoCase(key, "Port"))
        {
          // remove any leading/trailing quotes
          if (!value.empty() && value[0] == '"')
            value = value.substr(1);
          if (!value.empty() && value[value.size() - 1] == '"')
            value = value.substr(0, value.size() - 1);

          if (StringUtils::EqualsNoCase(key, "Path"))
            cookie->path = value;
          else if (StringUtils::EqualsNoCase(key, "Domain"))
            cookie->domain = value;
          else if (StringUtils::EqualsNoCase(key, "Comment"))
            cookie->comment = value;
          else if (StringUtils::EqualsNoCase(key, "Expires"))
          {
            // parse Expires value as RFC 1123 date time (Wdy, DD-Mon-YY HH:MM:SS GMT)
            CDateTime expiresDateTime;
            if (expiresDateTime.SetFromRFC1123DateTime(value) && expiresDateTime.IsValid())
              cookie->expires = value;
          }
          else if (StringUtils::EqualsNoCase(key, "CommentURL"))
            cookie->commentURL = value;
          else if (StringUtils::EqualsNoCase(key, "Port"))
            cookie->port = value;
        }
        // check if we found a boolean attribute
        else if (StringUtils::EqualsNoCase(key, "Secure"))
          cookie->secure = true;
        else if (StringUtils::EqualsNoCase(key, "Discard"))
          cookie->discard = true;
        else if (StringUtils::EqualsNoCase(key, "HttpOnly"))
          cookie->httponly = true;
        // check if we found an integer attribute
        else if (StringUtils::EqualsNoCase(key, "Version"))
        {
          if (StringUtils::IsInteger(value))
          {
            cookie->version = static_cast<int>(strtol(value.c_str(), NULL, 10));
            if (cookie->version < 1)
              cookie->version = 1;
          }
        }
        else if (StringUtils::EqualsNoCase(key, "Max-Age"))
        {
          // the first character must be a digit or a '-'
          // and the rest must be digits
          if (StringUtils::IsInteger(value))
          {
            cookie->updateExpires(strtol(value.c_str(), NULL, 10));
            cookie->max_age_default = cookie->max_age;
          }
        }
        else
        {
          // ignore cookies starting with a '$' (see RFC2965)
          if (!cookie->name.empty() && cookie->name[0] == '$')
            delete cookie;
          // add the parsed cookie to the list
          else
            cookies.insert(std::make_pair(cookie->name, cookie));

          // and create a new one
          cookie = new Cookie(key, value);
        }
      }

      if (cookie != NULL)
      {
        // ignore cookies starting with a '$' (see RFC2965)
        if (!cookie->name.empty() && cookie->name[0] == '$')
          delete cookie;
        // add the parsed cookie to the list
        else
          cookies.insert(std::make_pair(cookie->name, cookie));
      }

      return cookies;
    }

    void Cookie::updateExpires(long max_age)
    {
      max_age = max_age;
      max_age_set = true;

      time_t timestamp = 0;
      if (max_age > 0)
      {
        // get the current UTC/GMT timestamp value
        CDateTime::GetUTCDateTime().GetAsTime(timestamp);

        // add the max_age to it but make sure we don't overflow
        int max_to_overflow = std::numeric_limits<long>::max() - timestamp;
        if (max_age > max_to_overflow)
          timestamp = std::numeric_limits<long>::max();
        else
          timestamp += max_age;
      }

      CDateTime expiresTime(timestamp);
      if (!expiresTime.IsValid())
        return;

      // set expires as RFC 1123 datetime (Wdy, DD-Mon-YY HH:MM:SS GMT)
      expires = expiresTime.GetAsRFC1123DateTime();
    }
  }
}

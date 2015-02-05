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

#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/AddonString.h"
#include "interfaces/legacy/Exception.h"
#include "interfaces/legacy/swighelper.h"
#include "interfaces/legacy/mod_python/HttpRequest.h"

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    class Cookie : public AddonClass
    {
    public:
      Cookie(String name, String value) throw(InvalidArgumentException);
      virtual ~Cookie();

      // methods matching Apache's mod_python
      XBMCAddon::Dictionary<Cookie*> parse(String data);

      static void add_cookie(HttpRequest *request, Cookie *cookie) throw(NullPointerException);

      static XBMCAddon::Dictionary<Cookie*> get_cookies(HttpRequest *request) throw(NullPointerException);

      static Cookie* get_cookie(HttpRequest *request, String name) throw(NullPointerException);

      // custom methods
      std::string toString();

      // members matching Apache's mod_python
      /**
       * String. The name of the cookie. Names that begin with $ are reserved for other uses and must not be used by applications. (Required; Read-Only)
       */
      SWIG_IMMUTABLE(String name);
      /**
       * String. The value of the cookie is opaque to the user agent and may be anything the origin server chooses to send,
       * possibly in a server-selected printable ASCII encoding. (Required)
       */
      String value;
      /**
       * Integer. The Version attribute, a decimal integer, identifies to which version of the state management specification the cookie conforms.
       */
      int version;
      /**
       * String. The Path attribute specifies the subset of URLs to which this cookie applies. (Required)
       */
      String path;
      /**
       * String. The Domain attribute specifies the domain for which the cookie is valid.  An explicitly specified domain must always start with a dot.
       */
      String domain;
      /**
       * Boolean. The Secure attribute directs the user agent to use only (unspecified) secure means to contact the origin server whenever it sends back this cookie.
       */
      bool secure;
      /**
       * String. Because cookies can contain private information about a user, the Cookie attribute allows an origin server to document its intended use of a cookie.
       * The user can inspect the information to decide whether to initiate or continue a session with this cookie.
       */
      String comment;
      /**
       * String. The expires date value follows the format "Wdy, DD-Mon-YY HH:MM:SS GMT" (with or without surrounding quotes)
       * and specifies at what point in time the cookie expires and must be discarded. (Read-Only)
       */
      SWIG_IMMUTABLE(String expires);
      /**
       * Long Integer. The Max-Age attribute defines the lifetime of the cookie, in seconds. The delta-seconds value is a decimal non-negative integer.
       * After delta-seconds seconds elapse, the client should discard the cookie. A value of zero means the cookie should be discarded immediately.
       */
      long max_age;
      /**
       * String. Because cookies can be used to derive or store private information about a user, the CommentURL attribute allows an origin server to document how it intends to use the cookie.
       * The user can inspect the information identified by the URL to decide whether to initiate or continue a session with this cookie.
       */
      String commentURL;
      /**
       * Boolean. The Discard attribute instructs the user agent to discard the cookie unconditionally when the user agent terminates.
       */
      bool discard;
      /**
       * String. The Port attribute restricts the port to which a cookie may be returned in a Cookie request header.
       * Note that the syntax requires quotes around the optional portlist even if there is only  one portnum in portlist.
       */
      String port;
      /**
       * Boolean. The HttpOnly attribute limits the scope of the cookie to HTTP requests. In particular, the attribute instructs the user agent to
       * omit the cookie when providing access to cookies via "non-HTTP" APIs (such as a web browser API that exposes cookies to scripts).
       */
      bool httponly;

#ifndef SWIG
    protected:
      static XBMCAddon::Dictionary<Cookie*> parseCookies(String data);

      void updateExpires(long max_age);

      bool max_age_set;
      long max_age_default;
#endif
    };    
  }
}



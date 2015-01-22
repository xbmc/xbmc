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

#include <map>
#include <string>

#ifndef SWIG
#include "XBDateTime.h"
#endif
#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/AddonString.h"
#include "interfaces/legacy/Exception.h"
#include "interfaces/legacy/swighelper.h"
#include "interfaces/legacy/mod_python/HttpRequest.h"
#include "utils/Variant.h"

#ifndef SWIG
#define SESSION_TIMEOUT_DEFAULT 1800
#endif

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    class Session : public AddonClass
    {
    public:
#ifndef SWIG
      Session(const String& sessionId);
      Session(const Session* session) throw(NullPointerException);
#endif

      Session(HttpRequest* request, const String& sid = "", int timeout = SESSION_TIMEOUT_DEFAULT) throw(NullPointerException);
      virtual ~Session();

      // methods matching Apache's mod_python
      /**
       * Returns true if this session is new. A session will also be ``new''
       * after an attempt to instantiate an expired or non-existent session.
       * It is important to use this method to test whether an attempt to
       * instantiate a session has succeeded.
       */
      inline bool is_new() { return m_isNew; }

      /**
       * Returns the session id.
       */
      inline const String& id() { return m_sessionId; }

      /**
       * Returns the session creation time in seconds since beginning of epoch.
       */
      int created();

      /**
       * Returns last access time in seconds since beginning of epoch.
       */
      int last_accessed();

      /**
       * Returns session timeout interval in seconds.
       */
      int timeout() { return m_timeout; }

      /**
       * Set timeout to secs.
       */
      void set_timeout(int seconds) throw(InvalidArgumentException);

      /**
       * This method will remove the session from the persistent store and also place a header in outgoing headers to invalidate the session id cookie.
       */
      void invalidate();

      /**
       * Load the session values from storage.
       */
      bool load();

      /**
       * This method writes session values to storage.
       */
      bool save();

      /**
       * Remove the session from storage.
       */
      void del();

      // methods necessary to support using the Session like a dictionary
      /**
      * operator[key] -- Gets the item with the given key.\n
      * \n
      * key          : string or unicode - the key of an item.\n
      */
      inline CVariant& operator[](const String& key) { return m_map[key]; }

      /**
      * size() -- Returns the number of items.\n
      */
      inline int size() { return static_cast<int>(m_map.size()); }

      /**
      * contains(key) -- Checks if an item with the given key exists.\n
      * \n
      * key          : string or unicode - the key of an item.\n
      */
      inline bool contains(const String& key) { return m_map.find(key) != m_map.end(); }

      // custom methods
      String toString() const;

#ifndef SWIG
      bool IsValid(bool validateRequest = false) const;
      void Copy(const Session* session);

      HttpRequest* m_request;

      String m_sessionId;
      bool m_isNew;

      CDateTime m_created;
      CDateTime m_lastAccessed;
      int m_timeout;

      std::map<String, CVariant> m_map;
#endif
    };    
  }
}



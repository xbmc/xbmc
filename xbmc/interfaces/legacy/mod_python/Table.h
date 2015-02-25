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

#include "commons/Exception.h"
#include "interfaces/legacy/AddonClass.h"
#include "interfaces/legacy/Exception.h"

namespace XBMCAddon
{
  namespace xbmcmod_python
  {
    class Table : public AddonClass
    {
    public:
      inline Table() { }
#ifndef SWIG
      inline Table(const Table* table) throw(NullPointerException)
      {
        if (table == NULL)
          throw NullPointerException("table");

        m_map.insert(table->m_map.begin(), table->m_map.end());
      }
#endif
      virtual ~Table() { }

      /**
      * operator[key] -- Gets the item with the given key.\n
      * \n
      * key          : string or unicode - the key of an item.\n
      */
      inline String& operator[](const String& key) { return m_map.find(key)->second; }

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

      /**
      * get(key [, default) -- Gets the item with the given key or the default value if no item with the given key exists.\n
      * \n
      * key          : string or unicode - the key of an item.\n
      * default      : [opt] string or unicode - default value returned if the given key doesn't exist.\n
      */
      inline String get(const String& key, const String& defaultValue = "")
      {
        std::map<String, String>::const_iterator it = m_map.find(key);
        if (it != m_map.end())
          return it->second;

        return defaultValue;
      }

      /**
      * clear() -- Removes all items.\n
      */
      inline void clear() { m_map.clear(); }

      /**
      * add(key value) -- Adds a new item with the given key and value.\n
      * \n
      * key          : string or unicode - the key of the new item.\n
      * value        : string or unicode - the value of the new item.\n
      */
      inline void add(const String& key, const String& value) { m_map.insert(std::make_pair(key, value)); }

#ifndef SWIG
      std::multimap<String, String> m_map;
#endif
    };
  }
}



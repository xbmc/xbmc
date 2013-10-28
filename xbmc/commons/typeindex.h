/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <typeinfo>

/**
 * This struct represents a pre-introduction of the std::type_index for RTTI
 *  which is only availalbe in C++11.
 */

namespace XbmcCommons
{
  /**
     @brief The class type_index provides a simple wrapper for type_info
     which can be used as an index type in associative containers (23.6)
     and in unordered associative containers (23.7).
   */
  struct type_index
  {
    inline type_index(const std::type_info& __rhs)
    : _M_target(&__rhs) { }

    inline bool
    operator==(const type_index& __rhs) const
    { return *_M_target == *__rhs._M_target; }

    inline bool
    operator!=(const type_index& __rhs) const
    { return *_M_target != *__rhs._M_target; }

    inline bool
    operator<(const type_index& __rhs) const
    { return _M_target->before(*__rhs._M_target) != 0; }

    inline bool
    operator<=(const type_index& __rhs) const
    { return !__rhs._M_target->before(*_M_target); }

    inline bool
    operator>(const type_index& __rhs) const
    { return __rhs._M_target->before(*_M_target) != 0; }

    inline bool
    operator>=(const type_index& __rhs) const
    { return !_M_target->before(*__rhs._M_target); }

    inline const char*
    name() const
    { return _M_target->name(); }

  private:
    const std::type_info* _M_target;
  };

  template<typename _Tp> struct hash;
}

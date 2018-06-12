/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

namespace XbmcThreads
{
  /**
   * This will create a new predicate from an old predicate P with
   *  inverse truth value. This predicate is safe to use in a
   *  TightConditionVariable<P>
   */
  template <class P> class InversePredicate
  {
    P predicate;

  public:
    inline explicit InversePredicate(P predicate_) : predicate(predicate_) {}
    inline InversePredicate(const InversePredicate<P>& other) : predicate(other.predicate) {}
    inline InversePredicate<P>& operator=(InversePredicate<P>& other) { predicate = other.predicate; }

    inline bool operator!() const { return !(!predicate); }
  };

}


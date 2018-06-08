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

namespace XBMCAddon
{
  namespace Python
  {
    class PyGILLock;

    /**
     * These classes should NOT be used with 'new'. They are expected to reside
     *  as stack instances and they act as "Guard" classes that track the
     *  current context.
     */
    class PyContext
    {
    protected:
      friend class PyGILLock;
      static void* enterContext();
      static void leaveContext();
    public:

      inline PyContext() { enterContext(); }
      inline ~PyContext() { leaveContext(); }
    };

    /**
     * This class supports recursive locking of the GIL. It assumes that
     * all Python GIL manipulation is done through this class so that it
     * can monitor the current owner.
     */
    class PyGILLock
    {
    public:
      static void releaseGil();
      static void acquireGil();

      inline PyGILLock() { releaseGil(); }
      inline ~PyGILLock() { acquireGil(); }
    };
  }
}

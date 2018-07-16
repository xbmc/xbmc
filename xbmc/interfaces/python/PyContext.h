/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

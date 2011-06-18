/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

namespace XbmcThreads
{
  /**
   * This interface is meant to be implemented by operations that
   *  block and can be interrupted.
   *
   * The implementer of an IInterruptible must not only handle the Interrupt
   *  callback but also register and unregister the Interruptable before entering
   *  and after leaving a wait state. The Guard class is a helper for the 
   *  implementers to do this. As an example:
   *
   *  class MyInterruptible : public IInterruptible
   *  {
   *  public:
   *     virtual void Interrupt();
   *
   *     void blockingMethodCall() { XbmcThreads::IInterruptible::Guard g(*this); ... do blocking; }
   *  };
   *
   * See CEvent as an example
   */
  class IInterruptible
  {
  public:
    virtual void Interrupt() = 0;

    /**
     * Calling InterruptAll will invoke interrup on all IInterruptibles
     *  currently in a wait state
     */
    static void InterruptAll();

    /**
     * Calling InterruptThreadSpecific will invoke interrup on all IInterruptibles
     *  currently in a wait state in this thread only.
     */
    static void InterruptThreadSpecific();

  protected:

    void enteringWaitState();
    void leavingWaitState();

    class Guard
    {
      IInterruptible* interruptible;
    public:
      inline Guard(IInterruptible* pinterruptible) : interruptible(pinterruptible) { if (pinterruptible) pinterruptible->enteringWaitState(); }
      inline ~Guard() { if (interruptible) interruptible->leavingWaitState(); }
    };
  };
}


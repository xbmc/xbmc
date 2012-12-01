/*
 *      Copyright (C) 2005-2012 Team XBMC
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

/**
 * Defining LOG_LIFECYCLE_EVENTS will log all instantiations, deletions
 *  and also reference countings (increments and decrements) that take
 *  place on any Addon* class.
 *
 * Comment out (or uncomment out) to change the setting.
 */
//#define LOG_LIFECYCLE_EVENTS

/** 
 * Defining XBMC_ADDON_DEBUG_MEMORY will make the Acquire and Release
 *  methods virtual allow the developer to overload them in a sub-class
 *  and set breakpoints to aid in debugging. It will also cause the 
 *  reference counting mechanism to never actually delete any AddonClass
 *  instance allowing for the tracking of more references to (supposedly)
 *  deallocated classes.
 *
 * Comment out (or uncomment out) to change the setting.
 */
//#define XBMC_ADDON_DEBUG_MEMORY

#include "AddonString.h"
#include "threads/SingleLock.h"
#include "threads/Atomics.h"
#include "utils/log.h"

#include "AddonUtils.h"

namespace XBMCAddon
{
  class LanguageHook;

  /**
   * This class is the superclass for all reference counted classes in the api.
   * It provides a means for the bindings to handle all api objects generically.
   *
   * It also provides some means for debugging "lifecycle" events (see the above
   *  description of LOG_LIFECYCLE_EVENTS).
   *
   * If a scripting language bindings require specific handling there is a 
   *  hook to add in these language specifics that can be set here.
   */
  class AddonClass
  {
  private:
    long   refs;
    String classname;
    CCriticalSection thisLock;
    bool m_isDeallocating;

#ifdef XBMC_ADDON_DEBUG_MEMORY
    bool isDeleted;
#endif

    friend class Synchronize;
  protected:
    LanguageHook* languageHook;

    /**
     * This method is meant to be called from the destructor of the
     *  lowest level class.
     *
     * It's virtual because it's a conveinent place to receive messages that
     *  we're about to go be deleted but prior to any real tear-down.
     *
     * Any overloading classes need to remember to pass the call up the chain.
     */
    virtual void deallocating()
    {
      Synchronize lock(*this);
      m_isDeallocating = true;
    }

    // no copying
    inline AddonClass(const AddonClass& other) {}

  public:
    AddonClass(const char* classname);
    virtual ~AddonClass();

    /**
     * This method should be called while holding a Synchronize
     *  on the object. It will prevent the deallocation during
     *  the time it's held.
     */
    bool isDeallocating() { TRACE; return m_isDeallocating; }

#ifdef XBMC_ADDON_DEBUG_MEMORY
    virtual 
#else
    inline
#endif
    void Release() const
#ifndef XBMC_ADDON_DEBUG_MEMORY
    {
      long ct = AtomicDecrement((long*)&refs);
#ifdef LOG_LIFECYCLE_EVENTS
      CLog::Log(LOGDEBUG,"NEWADDON REFCNT decrementing to %ld on %s 0x%lx", ct,classname.c_str(), (long)(((void*)this)));
#endif
      if(ct == 0)
        delete this;
    }
#else
    ;
#endif


#ifdef XBMC_ADDON_DEBUG_MEMORY
    virtual 
#else
    inline
#endif
    void Acquire() const
#ifndef XBMC_ADDON_DEBUG_MEMORY
    {
#ifdef LOG_LIFECYCLE_EVENTS
      CLog::Log(LOGDEBUG,"NEWADDON REFCNT incrementing to %ld on %s 0x%lx", 
                AtomicIncrement((long*)&refs),classname.c_str(), (long)(((void*)this)));
#else
      AtomicIncrement((long*)&refs);
#endif
    }
#else
    ;
#endif

#define refcheck
    /**
     * This class is a smart pointer for a Referenced class.
     */
    template <class T> class Ref
    {
      T * ac;
    public:
      inline Ref() : ac(NULL) {}
      inline Ref(const T* _ac) : ac((T*)_ac) { if (ac) ac->Acquire(); refcheck; }

      // copy semantics
      inline Ref(Ref<T> const & oref) : ac((T*)(oref.get())) { if (ac) ac->Acquire(); refcheck; }
      template<class O> inline Ref(Ref<O> const & oref) : ac(static_cast<T*>(oref.get())) { if (ac) ac->Acquire(); refcheck; }

      /**
       * operator= should work with either another smart pointer or a pointer since it will
       * be able to convert a pointer to a smart pointer using one of the above constuctors.
       *
       * Note: There is a trick here. The temporary variable is necessary because otherwise the
       * following code will fail:
       *
       * Ref<T> ptr = new T;
       * ptr = ptr;
       *
       * What happens without the tmp is the dereference is called first so the object ends up
       * deleted and then the reference happens on a deleted object. The order is reversed
       * in the following.
       *
       * Note: Operator= is ambiguous if you define both an operator=(Ref<T>&) and an operator=(T*). I'm
       * opting for the route the boost took here figuring it has more history behind it.
       */
      inline Ref<T>& operator=(Ref<T> const & oref)
      { T* tmp = ac; ac=((T*)oref.get()); if (ac) ac->Acquire(); if (tmp) tmp->Release(); refcheck; return *this; }

      inline T* operator->() const { refcheck; return ac; }

      /**
       * This operator doubles as the value in a boolean expression.
       */
      inline operator T*() const { refcheck; return ac; }
      inline T* get() const { refcheck; return ac; }
      inline T& getRef() const { refcheck; return *ac; }

      inline ~Ref() { refcheck; if (ac) ac->Release(); }
      inline bool isNull() const { refcheck; return ac == NULL; }
      inline bool isNotNull() const { refcheck; return ac != NULL; }
      inline bool isSet() const { refcheck; return ac != NULL; }
      inline bool operator!() const { refcheck; return ac == NULL; }

      // This is there only for boost compatibility
      template<class O> inline void reset(Ref<O> const & oref) { refcheck; (*this) = static_cast<T*>(oref.get()); refcheck; }
      template<class O> inline void reset(O * oref) { refcheck; (*this) = static_cast<T*>(oref); refcheck; }
      inline void reset() { refcheck; if (ac) ac->Release(); ac = NULL; }
    };

    /**
     * This class can be used like a "synchronize" block in java as long
     *  as the object is an AddonClass. It can be used to synchronize on
     *  'this' effectively creating the effect of a synchronize keyword
     *  on a method declaration.
     *
     * Keep in mind that this DOES NOT use 'monitor' semantics, but 
     *  uses MUTEX semantics. That means that using this class, a thread
     *  can deadlock itself, while in java a synchronize keyword won't.
     */
    class Synchronize : public CSingleLock
    {
    public:
      inline Synchronize(const AddonClass& obj) : CSingleLock(obj.thisLock) {}
    };

  };
}

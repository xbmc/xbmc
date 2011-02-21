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

#include <stddef.h>

// Comment in this #define in order to log REFCNT errors.
// WARNING! defining DEBUG_REFS prevents the Referenced objects from EVER
// being deleted. They will ALL LEAK! This is just for debugging.
//#define DEBUG_REFS
#ifdef DEBUG_REFS
#define refcheck if (ac != NULL) ac->Referenced::check()
#else
#define refcheck
#endif

namespace xbmcutil
{
  /**
   * This class is the superclass for all reference counted classes.
   */
  class Referenced
  {
  private:
    long   refs;

  public:
    Referenced() : refs(0) {}
    virtual ~Referenced();

    void Release() const;
    void Acquire() const;

#ifdef DEBUG_REFS
    void check() const;
#endif

    /**
     * This class is a smart pointer for a Referenced class.
     */
    template <class T> class ref
    {
      T * ac;
    public:
      typedef T*(*factory)();

      inline ref(factory factoryMethod) : ac( (*factoryMethod)() ) { if (ac) ac->Acquire(); refcheck; }
      inline ref() : ac(NULL) {}
      inline ref(const T* _ac) : ac((T*)_ac) { if (ac) ac->Acquire(); refcheck; }

      // copy semantics
      inline ref(ref<T> const & oref) : ac((T*)(oref.get())) { if (ac) ac->Acquire(); refcheck; }
      template<class O> inline ref(ref<O> const & oref) : ac(static_cast<T*>(oref.get())) { if (ac) ac->Acquire(); refcheck; }

      /**
       * operator= should work with either another smart pointer or a pointer since it will 
       *  be able to convert a pointer to a smart pointer using one of the above constuctors.
       *
       * Note: There is a trick here. The temporary varialbe is necessary because otherwise the 
       *  following code will fail:
       *
       *  ref<T> ptr = new T;
       *  ptr = ptr;
       *
       * What happens without the tmp is the derefernce is called first so the object ends up
       *  deleted and then the reference happens on a deleted object. The order is reversed
       *  in the following.
       *
       * Note: Operator= is ambiguous if you define both an operator=(ref<T>&) and an operator=(T*). I'm
       *  opting for the route the boost took here figuring it has more history behind it.
       */
      inline ref<T>& operator=(ref<T> const & oref)  
      { T* tmp = ac; ac=((T*)oref.get()); if (ac) ac->Acquire(); if (tmp) tmp->Release(); refcheck; return *this; }

      inline T* operator->() const { refcheck; return ac; }

      /**
       * This operator doubles as the value in a boolean expression.
       */
      inline operator T*() const { refcheck; return ac; }
      inline T* get() const { refcheck; return ac; }
      inline T& getRef() const { refcheck; return *ac; }

      inline ~ref() { refcheck; if (ac) ac->Release(); }
      inline bool isNull() const { refcheck; return ac == NULL; }
      inline bool isNotNull() const { refcheck; return ac; }
      inline bool isSet() const { refcheck; return ac; }
      inline bool operator!() const { refcheck; return ac == NULL; }

      // This is there only for boost compatibility
      template<class O> inline void reset(ref<O> const & oref) { refcheck; (*this) = static_cast<T*>(oref.get()); refcheck; }
      template<class O> inline void reset(O * oref) { refcheck; (*this) = static_cast<T*>(oref); refcheck; }
      inline void reset() { refcheck; if (ac) ac->Release(); ac = NULL; }
    };

  };

  /**
   * Currently THIS IS NOT THREAD SAFE! Why not just add a lock you ask?
   *  Because this singleton is used to initialize global variables and
   *  there is an issue with having the lock used prior to its 
   *  initialization. No matter what, if this class is used as a replacement
   *  for global variables there's going to be a race condition if it's used
   *  anywhere else. So currently this is the only prescribed use.
   *
   * Therefore this hack depends on the fact that compilation unit global/static 
   *  initialization is done in a single thread.
   *
   * In the spirit of making changes incrementally I've opted to not add Atomic*
   *  locking to this class yet. It doesn't need it (yet) as it's only called
   *  from the static initialization thread prior to main.
   */
  template <class T> class Singleton
  {
    static T* instance;
  public:
    static T* getInstance()
    {
      if (instance == NULL)
        instance = new T;
      return instance;
    }
  };

  template <class T> T* Singleton<T>::instance;

}

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

#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"

namespace xbmcutil
{
  /**
   * This class is the superclass for all reference counted classes.
   */
  class Referenced
  {
  private:
    long   refs;
//    std::string classname;

  public:
//    Referenced(const char* _classname) : refs(0), classname(_classname) {}
    Referenced() : refs(0) {}
    virtual ~Referenced();

    void Release() const;
    void Acquire() const;

    /**
     * This class is a smart pointer for a Referenced class.
     */
    template <class T> class ref
    {
      T * ac;
    public:
      typedef T*(*factory)();

      inline ref(factory factoryMethod) : ac( (*factoryMethod)() ) { if (ac) ac->Acquire(); }
      inline ref() : ac(NULL) {}
      inline ref(const T* addonClass) : ac((T*)addonClass) { if (ac) ac->Acquire(); }

      // copy semantics
      inline ref(const ref<T>& oref) : ac((T*)(oref.get())) { if (ac) ac->Acquire(); }
      inline ref<T>& operator=(const ref<T>& oref)  
      { if (ac) ac->Release(); ac=((T*)oref.get()); if (ac) ac->Acquire(); return *this; }

      inline ref<T>& operator=(T* addonClass) { if (ac) ac->Release(); ac = addonClass; if (ac) ac->Acquire(); return *this; }
      inline ref<T>& operator=(const T* addonClass) const 
      { if (ac) ac->Release(); ac = addonClass; if (ac) ac->Acquire(); return *this; }

      inline T* operator->() { return ac; }
      inline const T* operator->() const { return ac; }
      inline operator T*() { return ac; }
      inline operator const T*() const { return ac; }
      inline const T* get() const { return ac; }
      inline T* get() { return ac; }

      inline ~ref() { if (ac) ac->Release(); }
      inline bool isNull() const { return ac == NULL; }
      inline bool isNotNull() const { return ac; }
      inline bool isSet() const { return ac; }
      inline bool operator!() const { return ac != NULL; }
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

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

#include "AddonClass.h"
#ifdef XBMC_ADDON_DEBUG_MEMORY
#include "utils/log.h"
#endif
#include "LanguageHook.h"
#include "AddonUtils.h"

using namespace XBMCAddonUtils;

namespace XBMCAddon
{
  // need a place to put the vtab
  AddonClass::~AddonClass()
  {
    m_isDeallocating= true;

    if (languageHook != NULL)
      languageHook->Release();

#ifdef XBMC_ADDON_DEBUG_MEMORY
    isDeleted = false;
#endif
  }

  AddonClass::AddonClass() : refs(0L), m_isDeallocating(false), 
                             languageHook(NULL)
  {
#ifdef XBMC_ADDON_DEBUG_MEMORY
    isDeleted = false;
#endif

    // check to see if we have a language hook that was prepared for this instantiation
    languageHook = LanguageHook::GetLanguageHook();
    if (languageHook != NULL)
    {
      languageHook->Acquire();

      // here we assume the language hook was set for the single instantiation of
      //  this AddonClass (actually - its subclass - but whatever). So we
      //  will now reset the Tls. This avoids issues if the constructor of the
      //  subclass throws an exception.
      LanguageHook::ClearLanguageHook();
    }
  }

#ifdef XBMC_ADDON_DEBUG_MEMORY
  void AddonClass::Release() const
  {
    if (isDeleted)
      CLog::Log(LOGERROR,"NEWADDON REFCNT Releasing dead class %s 0x%lx", 
                GetClassname(), (long)(((void*)this)));

    long ct = AtomicDecrement((long*)&refs);
#ifdef LOG_LIFECYCLE_EVENTS
    CLog::Log(LOGDEBUG,"NEWADDON REFCNT decrementing to %ld on %s 0x%lx", refs, GetClassname(), (long)(((void*)this)));
#endif
    if(ct == 0)
    {
        ((AddonClass*)this)->isDeleted = true;
        // we're faking a delete but not doing it so call the destructor explicitly
        this->~AddonClass();
    }
  }

  void AddonClass::Acquire() const
  {
    if (isDeleted)
      CLog::Log(LOGERROR,"NEWADDON REFCNT Acquiring dead class %s 0x%lx", 
                GetClassname(), (long)(((void*)this)));

#ifdef LOG_LIFECYCLE_EVENTS
    CLog::Log(LOGDEBUG,"NEWADDON REFCNT incrementing to %ld on %s 0x%lx", 
              AtomicIncrement((long*)&refs),GetClassname(), (long)(((void*)this)));
#else
    AtomicIncrement((long*)&refs);
#endif
  }
#endif
}

              

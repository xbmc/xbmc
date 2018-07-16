/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

  AddonClass::AddonClass() : refs(0L),
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

    long ct = --refs;
#ifdef LOG_LIFECYCLE_EVENTS
    CLog::Log(LOGDEBUG,"NEWADDON REFCNT decrementing to %ld on %s 0x%lx", refs.load(), GetClassname(), (long)(((void*)this)));
#endif
    if(ct == 0)
    {
        const_cast<AddonClass*>(this)->isDeleted = true;
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
              ++refs, GetClassname(), (long)(((void*)this)));
#else
    ++refs;
#endif
  }
#endif
}



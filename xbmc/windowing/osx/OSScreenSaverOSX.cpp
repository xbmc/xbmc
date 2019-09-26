/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "OSScreenSaverOSX.h"

#include <CoreFoundation/CoreFoundation.h>

void COSScreenSaverOSX::Inhibit()
{
  // see Technical Q&A QA1340
  if (m_assertionID == 0)
  {
    CFStringRef reasonForActivity= CFSTR("XBMC requested disable system screen saver");
    IOPMAssertionCreateWithName(kIOPMAssertionTypeNoDisplaySleep,
      kIOPMAssertionLevelOn, reasonForActivity, &m_assertionID);
  }
}

void COSScreenSaverOSX::Uninhibit()
{
  if (m_assertionID != 0)
  {
    IOPMAssertionRelease(m_assertionID);
    m_assertionID = 0;
  }
}
#pragma once
/*
 * addon_main.h — Kodi ADSP addon entry point declarations
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 *
 * All required Kodi ADSP C callback functions are declared here.
 * Implementations are in addon_main.cpp.
 * The get_addon() function defined in kodi_adsp_dll.h wires them all.
 *
 * Global addon instance (one per addon load, shared across streams via
 * per-stream handles).  Per-stream state is stored in DSPProcessor objects
 * pointed to by ADDON_HANDLE::dataAddress.
 */

#include "kodi_adsp_dll.h"   // Kodi will find this via include path

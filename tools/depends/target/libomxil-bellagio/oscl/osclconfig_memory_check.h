/* ------------------------------------------------------------------
 * Copyright (C) 1998-2009 PacketVideo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 * -------------------------------------------------------------------
 */

#ifndef OSCLCONFIG_MEMORY_CHECK_H_INCLUDED
#define OSCLCONFIG_MEMORY_CHECK_H_INCLUDED


/**
OSCL_BYPASS_MEMMGT macro should be set to 1 if
it is desirable to bypass the PV memory management system and just
use the native memory management.
Otherwise it should be set to 0.
*/
#ifndef OSCL_BYPASS_MEMMGT
#error "ERROR: OSCL_BYPASS_MEMMGT has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_ANSI_MEMORY_FUNCS macro should be set to 1 if
the target platform supports ANSI C memory functions (malloc, free, etc).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_ANSI_MEMORY_FUNCS
#error "ERROR: OSCL_HAS_ANSI_MEMORY_FUNCS has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_SYMBIAN_MEMORY_FUNCS macro should be set to 1 if
the target platform supports Symbian memory functions User::Alloc, User::Free, etc.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SYMBIAN_MEMORY_FUNCS
#error "ERROR: OSCL_HAS_SYMBIAN_MEMORY_FUNCS has to be defined to either 1 or 0"
#endif

/*
 * OSCL_HAS_HEAP_BASE_SUPPORT macro should be set to 1 for the
 * platforms that allows inheritance from HeapBase class for
 * overloading of new/delete operators.
 */

#ifndef OSCL_HAS_HEAP_BASE_SUPPORT
#error "ERROR: OSCL_HAS_HEAP_BASE_SUPPORT has to be defined to either 1 or 0."
#endif

/*
 * OSCL_HAS_GLOBAL_NEW_DELETE macro should be set to 1 for the
 * platforms that allows overloading of new/delete operators.
 */

#ifndef OSCL_HAS_GLOBAL_NEW_DELETE
#error "ERROR: OSCL_HAS_GLOBAL_NEW_DELETE has to be defined to either 1 or 0."
#endif

#endif // OSCLCONFIG_MEMORY_CHECK_H_INCLUDED



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

#ifndef OSCLCONFIG_ERROR_CHECK_H_INCLUDED
#define OSCLCONFIG_ERROR_CHECK_H_INCLUDED


/**
OSCL_HAS_EXCEPTIONS macro should be set to 1 if
the target platform supports C++ exceptions (throw, catch).
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_EXCEPTIONS
#error "ERROR: OSCL_HAS_EXCEPTIONS has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_EXCEPTIONS macro should be set to 1 if
the target platform supports the POSIX-compliant errno.h header file.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_ERRNO_H
#error "ERROR: OSCL_HAS_ERRNO_H has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_SYMBIAN_ERRORTRAP macro should be set to 1 if
the target platform has Symbian leave, trap, and cleanup stack support.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SYMBIAN_ERRORTRAP
#error "ERROR: OSCL_HAS_SYMBIAN_ERRORTRAP has to be defined to either 1 or 0"
#endif

/**
OSCL_HAS_SETJMP_H macro should be set to 1 if
the target platform supports the setjmp.h header file including
the setjmp and longjmp functions.
Otherwise it should be set to 0.
*/
#ifndef OSCL_HAS_SETJMP_H
#error "ERROR: OSCL_HAS_SETJMP_H has to be defined to either 1 or 0"
#endif


#endif //OSCLCONFIG_ERROR_CHECK_H_INCLUDED



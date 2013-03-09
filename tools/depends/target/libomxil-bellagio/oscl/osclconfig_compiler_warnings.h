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
// -*- c++ -*-
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

//       O S C L C O N F I G _ C O M P I L E R  _ W A R N I N G S

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig_compiler_warnings.h
 *  \brief This file contains the ability to turn off/on compiler warnings
 *
 */

// This macro enables the "#pragma GCC system_header" found in any header file that
// includes this config file.
// "#pragma GCC system_header" suppresses compiler warnings in the rest of that header
// file by treating the header as a system header file.
// For instance, foo.h has 30 lines, "#pragma GCC system_header" is inserted at line 10,
// from line 11 to the end of file, all compiler warnings are disabled.
// However, this does not affect any files that include foo.h.
//
#ifdef __GNUC__
#define OSCL_DISABLE_GCC_WARNING_SYSTEM_HEADER
#endif

#define OSCL_FUNCTION_PTR(x) (&x)


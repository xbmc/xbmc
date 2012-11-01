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

//                O S C L C O N F I G _ E R R O R

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =


/*! \file osclconfig_error.h
 *  \brief This file contains the common typedefs and header files needed to compile osclerror
 *
 */


#ifndef OSCLCONFIG_ERROR_H_INCLUDED
#define OSCLCONFIG_ERROR_H_INCLUDED

#ifndef OSCLCONFIG_H_INCLUDED
#include "osclconfig.h"
#endif

#define OSCL_HAS_EXCEPTIONS                     1
#define OSCL_HAS_ERRNO_H                        1
#define OSCL_HAS_SYMBIAN_ERRORTRAP      0
#define OSCL_HAS_SETJMP_H 1

// system header files
#include <setjmp.h>
#include <errno.h>


// confirm that all definitions have been defined
#include "osclconfig_error_check.h"

#endif // OSCLCONFIG_ERROR_H_INCLUDED


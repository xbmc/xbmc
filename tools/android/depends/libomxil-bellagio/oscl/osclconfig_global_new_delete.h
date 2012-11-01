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
#ifndef OSCLCONFIG_GLOBAL_NEW_DELETE_H_INCLUDED
#define OSCLCONFIG_GLOBAL_NEW_DELETE_H_INCLUDED

//This file contains overloads for the global new/delete operators
//for use in configurations without a native new/delete operator,
//or where it is desirable to overload the existing global new/delete
//operators.  The implementation of the operators is in oscl_mem.cpp.

void* operator new(size_t);
void operator delete(void*);


#endif

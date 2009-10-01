/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __WINS__

/* Disable some warnings */

#pragma warning(disable: 4100) /* unreferenced formal parameter */
#pragma warning(disable: 4127) /* conditional expression is constant */
#pragma warning(disable: 4189) /* local variable is initialized but not referenced */
#pragma warning(disable: 4244) /* conversion from '...' to '...', possible loss of data */
#pragma warning(disable: 4305) /* truncation from '...' to '...' */
#pragma warning(disable: 4505) /* unreferenced local function has been removed */
#pragma warning(disable: 4514) /* unreferenced inline function has been removed */
#pragma warning(disable: 4702) /* unreachable code */
#pragma warning(disable: 4701) /* local variable may be be used without having been initialized */
#pragma warning(disable: 4706) /* assignment within conditional expression */
#pragma warning(disable: 4761) /* integral size mismatch in argument: conversion supplied */

#endif

#endif /* ! CONFIG_H */

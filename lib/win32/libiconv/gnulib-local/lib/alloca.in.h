/* Memory allocation on the stack.
   Copyright (C) 1995, 1999, 2001-2007 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* When this file is included, it may be preceded only by preprocessor
   declarations.  Thanks to AIX.  Therefore we include it right after
   "config.h", not later.  */

/* Avoid using the symbol _ALLOCA_H here, as Bison assumes _ALLOCA_H
   means there is a real alloca function.  */
#ifndef _GL_ALLOCA_H
#define _GL_ALLOCA_H

/* alloca(N) returns a pointer (void* or char*) to N bytes of memory
   allocated on the stack, and which will last until the function returns.
   Use of alloca should be avoided:
     - inside arguments of function calls - undefined behaviour,
     - in inline functions - the allocation may actually last until the
       calling function returns,
     - for huge N (say, N >= 65536) - you never know how large (or small)
       the stack is, and when the stack cannot fulfill the memory allocation
       request, the program just crashes.
 */

#ifndef alloca
# ifdef __GNUC__
#   define alloca __builtin_alloca
# else
#  ifdef _MSC_VER
#   include <malloc.h>
#   define alloca _alloca
#  else
#   if HAVE_ALLOCA_H
#    include <alloca.h>
#   else
#    ifdef _AIX
 #pragma alloca
#    else
#     ifdef __hpux /* This section must match that of bison generated files. */
#      ifdef __cplusplus
extern "C" void *alloca (unsigned int);
#      else /* not __cplusplus */
extern void *alloca ();
#      endif /* not __cplusplus */
#     else /* not __hpux */
#      ifndef alloca
extern char *alloca ();
#      endif
#     endif /* __hpux */
#    endif
#   endif
#  endif
# endif
#endif

#endif /* _GL_ALLOCA_H */

// Copyright (C) 2006 Fokko Beekhof
// Email contact: Fokko.Beekhof@cui.unige.ch

// The OMPTL library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef OMPTL
#define OMPTL 1

#ifndef _OPENMP
  #define par_generate generate
#else

#ifdef OMPTL_NO_DEBUG
  #define OMPTL_ASSERT(X)
#else
  #include <cassert>
  #define OMPTL_ASSERT(X) assert(X)
#endif

// For debugging
#ifndef _OMPTL_DEBUG_NO_OMP
  #include <omp.h>
#else
  #define omp_get_max_threads() (2)
#endif

#endif /* ifndef _OPENMP */

struct _Pfunc
{
	static unsigned Pfunc()
	{
		#ifdef _OPENMP
		assert(omp_get_max_threads() > 0);
		return omp_get_max_threads();
		#else
		return 0;
		#endif
	}
};

#endif /* OMPTL */

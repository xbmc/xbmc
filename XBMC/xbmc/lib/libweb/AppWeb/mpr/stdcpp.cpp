///
///	@file stdcpp.cpp 
///	@brief Replacement routines for libstdc++. Used in libstdcpp.
/// @overview This module provides replacement routines for libstdc++. 
///		Appweb relies only minimally on the C++ runtime and so most 
///		of the overhead of libstdc++ is unnecessary.
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
/////////////////////////////////// Includes ///////////////////////////////////

#define 	UNSAFE_FUNCTIONS_OK 1

#include	"mpr.h"

#if !BLD_FEATURE_STDCPP
////////////////////////////// Forward Declarations ////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

#if !BLD_FEATURE_MALLOC_HOOK
void *operator new(size_t size)
{
	void	*ptr;

	ptr = malloc(size);

	return ptr;
}

////////////////////////////////////////////////////////////////////////////////

void *operator new[](size_t size)
{
	void	*ptr;

	ptr = malloc(size);
	return ptr;
}

////////////////////////////////////////////////////////////////////////////////

void operator delete(void *ptr)
{
	free(ptr);
}

////////////////////////////////////////////////////////////////////////////////

void operator delete[](void *ptr)
{
	free(ptr);
}

#endif // !BLD_FEATURE_MALLOC_HOOK
////////////////////////////////////////////////////////////////////////////////
//
//	Do nothing when a pure virtual function is called
//

int __cxa_pure_virtual()
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

#if !MACOSX && (!BLD_FEATURE_STATIC_LINK_LIBC || __UCLIBC__)
int __cxa_atexit()
{
	return 0;
}
#endif


////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} // extern "C"
#endif

#else // BLD_FEATURE_STDCPP

void dummyStdcpp() {}
#endif

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//

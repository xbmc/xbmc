// =============================================================================
//  FILE:  StdString.h
//  AUTHOR:	Joe O'Leary (with outside help noted in comments)
//  REMARKS:
//		This header file declares the CStdStr template.  This template derives
//		the Standard C++ Library basic_string<> template and add to it the
//		the following conveniences:
//			- The full MFC CString set of functions (including implicit cast)
//			- writing to/reading from COM IStream interfaces
//			- Functional objects for use in STL algorithms
//
//		From this template, we intstantiate two classes:  CStdStringA and
//		CStdStringW.  The name "CStdString" is just a #define of one of these,
//		based upone the _UNICODE macro setting
//
//		This header also declares our own version of the MFC/ATL UNICODE-MBCS
//		conversion macros.  Our version looks exactly like the Microsoft's to
//		facilitate portability.
//
//	NOTE:
//		If you you use this in an MFC or ATL build, you should include either
//		afx.h or atlbase.h first, as appropriate.
//
//	PEOPLE WHO HAVE CONTRIBUTED TO THIS CLASS:
//
//		Several people have helped me iron out problems and othewise improve
//		this class.  OK, this is a long list but in my own defense, this code
//		has undergone two major rewrites.  Many of the improvements became
//		necessary after I rewrote the code as a template.  Others helped me
//		improve the CString facade.
//
//		Anyway, these people are (in chronological order):
//
//			- Pete the Plumber (???)
//			- Julian Selman
//			- Chris (of Melbsys)
//			- Dave Plummer
//			- John C Sipos
//			- Chris Sells
//			- Nigel Nunn
//			- Fan Xia
//			- Matthew Williams
//			- Carl Engman
//			- Mark Zeren
//			- Craig Watson
//			- Rich Zuris
//			- Karim Ratib
//			- Chris Conti
//			- Baptiste Lepilleur
//			- Greg Pickles
//			- Jim Cline
//			- Jeff Kohn
//			- Todd Heckel
//			- Ullrich Pollähne
//			- Joe Vitaterna
//			- Joe Woodbury
//			- Aaron (no last name)
//			- Joldakowski (???)
//			- Scott Hathaway
//			- Eric Nitzche
//			- Pablo Presedo
//
//	REVISION HISTORY
//	  2001-APR-27 - StreamLoad was calculating the number of BYTES in one
//					case, not characters.  Thanks to Pablo Presedo for this.
//
//    2001-FEB-23 - Replace() had a bug which caused infinite loops if the
//					source string was empty.  Fixed thanks to Eric Nitzsche.
//
//    2001-FEB-23 - Scott Hathaway was a huge help in providing me with the
//					ability to build CStdString on Sun Unix systems.  He
//					sent me detailed build reports about what works and what
//					does not.  If CStdString compiles on your Unix box, you
//					can thank Scott for it.
//
//	  2000-DEC-29 - Joldakowski noticed one overload of Insert failed to do
//					range check as CString's does.  Now fixed -- thanks!
//
//	  2000-NOV-07 - Aaron pointed out that I was calling static member
//					functions of char_traits via a temporary.  This was not
//					technically wrong, but it was unnecessary and caused
//					problems for poor old buggy VC5.  Thanks Aaron!
//
//	  2000-JUL-11 - Joe Woodbury noted that the CString::Find docs don't match
//					what the CString::Find code really ends up doing.   I was
//					trying to match the docs.  Now I match the CString code
//				  - Joe also caught me truncating strings for GetBuffer() calls
//					when the supplied length was less than the current length.
//
//	  2000-MAY-25 - Better support for STLPORT's Standard library distribution
//				  - Got rid of the NSP macro - it interfered with Koenig lookup
//				  - Thanks to Joe Woodbury for catching a TrimLeft() bug that
//					I introduced in January.  Empty strings were not getting
//					trimmed
//
//	  2000-APR-17 - Thanks to Joe Vitaterna for pointing out that ReverseFind
//					is supposed to be a const function.
//
//	  2000-MAR-07 - Thanks to Ullrich Pollähne for catching a range bug in one
//					of the overloads of assign.
//
//    2000-FEB-01 - You can now use CStdString on the Mac with CodeWarrior!
//					Thanks to Todd Heckel for helping out with this.
//
//	  2000-JAN-23 - Thanks to Jim Cline for pointing out how I could make the
//					Trim() function more efficient.
//				  - Thanks to Jeff Kohn for prompting me to find and fix a typo
//					in one of the addition operators that takes _bstr_t.
//				  - Got rid of the .CPP file -  you only need StdString.h now!
//
//	  1999-DEC-22 - Thanks to Greg Pickles for helping me identify a problem
//					with my implementation of CStdString::FormatV in which
//					resulting string might not be properly NULL terminated.
//
//	  1999-DEC-06 - Chris Conti pointed yet another basic_string<> assignment
//					bug that MS has not fixed.  CStdString did nothing to fix
//					it either but it does now!  The bug was: create a string
//					longer than 31 characters, get a pointer to it (via c_str())
//					and then assign that pointer to the original string object.
//					The resulting string would be empty.  Not with CStdString!
//
//	  1999-OCT-06 - BufferSet was erasing the string even when it was merely
//					supposed to shrink it.  Fixed.  Thanks to Chris Conti.
//				  - Some of the Q172398 fixes were not checking for assignment-
//					to-self.  Fixed.  Thanks to Baptiste Lepilleur.
//
//	  1999-AUG-20 - Improved Load() function to be more efficient by using 
//					SizeOfResource().  Thanks to Rich Zuris for this.
//				  - Corrected resource ID constructor, again thanks to Rich.
//				  - Fixed a bug that occurred with UNICODE characters above
//					the first 255 ANSI ones.  Thanks to Craig Watson. 
//				  - Added missing overloads of TrimLeft() and TrimRight().
//					Thanks to Karim Ratib for pointing them out
//
//	  1999-JUL-21 - Made all calls to GetBuf() with no args check length first.
//
//	  1999-JUL-10 - Improved MFC/ATL independence of conversion macros
//				  - Added SS_NO_REFCOUNT macro to allow you to disable any
//					reference-counting your basic_string<> impl. may do.
//				  - Improved ReleaseBuffer() to be as forgiving as CString.
//					Thanks for Fan Xia for helping me find this and to
//					Matthew Williams for pointing it out directly.
//
//	  1999-JUL-06 - Thanks to Nigel Nunn for catching a very sneaky bug in
//					ToLower/ToUpper.  They should call GetBuf() instead of
//					data() in order to ensure the changed string buffer is not
//					reference-counted (in those implementations that refcount).
//
//	  1999-JUL-01 - Added a true CString facade.  Now you can use CStdString as
//					a drop-in replacement for CString.  If you find this useful,
//					you can thank Chris Sells for finally convincing me to give
//					in and implement it.
//				  - Changed operators << and >> (for MFC CArchive) to serialize
//					EXACTLY as CString's do.  So now you can send a CString out
//					to a CArchive and later read it in as a CStdString.   I have
//					no idea why you would want to do this but you can. 
//
//	  1999-JUN-21 - Changed the CStdString class into the CStdStr template.
//				  - Fixed FormatV() to correctly decrement the loop counter.
//					This was harmless bug but a bug nevertheless.  Thanks to
//					Chris (of Melbsys) for pointing it out
//				  - Changed Format() to try a normal stack-based array before
//					using to _alloca().
//				  - Updated the text conversion macros to properly use code
//					pages and to fit in better in MFC/ATL builds.  In other
//					words, I copied Microsoft's conversion stuff again. 
//				  - Added equivalents of CString::GetBuffer, GetBufferSetLength
//				  - new sscpy() replacement of CStdString::CopyString()
//				  - a Trim() function that combines TrimRight() and TrimLeft().
//
//	  1999-MAR-13 - Corrected the "NotSpace" functional object to use _istpace()
//					instead of _isspace()   Thanks to Dave Plummer for this.
//
//	  1999-FEB-26 - Removed errant line (left over from testing) that #defined
//					_MFC_VER.  Thanks to John C Sipos for noticing this.
//
//	  1999-FEB-03 - Fixed a bug in a rarely-used overload of operator+() that
//					caused infinite recursion and stack overflow
//				  - Added member functions to simplify the process of
//					persisting CStdStrings to/from DCOM IStream interfaces 
//				  - Added functional objects (e.g. StdStringLessNoCase) that
//					allow CStdStrings to be used as keys STL map objects with
//					case-insensitive comparison 
//				  - Added array indexing operators (i.e. operator[]).  I
//					originally assumed that these were unnecessary and would be
//					inherited from basic_string.  However, without them, Visual
//					C++ complains about ambiguous overloads when you try to use
//					them.  Thanks to Julian Selman to pointing this out. 
//
//	  1998-FEB-?? - Added overloads of assign() function to completely account
//					for Q172398 bug.  Thanks to "Pete the Plumber" for this
//
//	  1998-FEB-?? - Initial submission
//
// COPYRIGHT:
//		1999 Joseph M. O'Leary.  This code is free.  Use it anywhere you want.
//		Rewrite it, restructure it, whatever.  Please don't blame me if it makes
//		your $30 billion dollar satellite explode in orbit.  If you redistribute
//		it in any form, I'd appreciate it if you would leave this notice here.
//
//		If you find any bugs, please let me know:
//
//				jmoleary@earthlink.net
//				http://home.earthlink.net/~jmoleary
// =============================================================================

// Avoid multiple inclusion the VC++ way,
// Turn off browser references
// Turn off unavoidable compiler warnings

#if defined(_MSC_VER) && (_MSC_VER > 1100)
	#pragma once
	#pragma component(browser, off, references, "CStdString")
	#pragma warning (disable : 4290) // C++ Exception Specification ignored
	#pragma warning (disable : 4127) // Conditional expression is constant
	#pragma warning (disable : 4097) // typedef name used as synonym for class name
#endif

#ifndef STDSTRING_H
#define STDSTRING_H

// MACRO: SS_NO_REFCOUNT:
//		turns off reference counting at the assignment level
//		I define this by default.  comment it out if you don't want it.

#define SS_NO_REFCOUNT	

// In non-Visual C++ and/or non-Win32 builds, we can't use some cool stuff.

#if !defined(_MSC_VER) || !defined(_WIN32)
	#define SS_ANSI
#endif

// Avoid legacy code screw up: if _UNICODE is defined, UNICODE must be as well

#if defined (_UNICODE) && !defined (UNICODE)
	#define UNICODE
#endif
#if defined (UNICODE) && !defined (_UNICODE)
	#define _UNICODE
#endif

// -----------------------------------------------------------------------------
// MIN and MAX.  The Standard C++ template versions go by so many names (at
// at least in the MS implementation) that you never know what's available 
// -----------------------------------------------------------------------------
template<class Type>
inline const Type& SSMIN(const Type& arg1, const Type& arg2)
{
	return arg2 < arg1 ? arg2 : arg1;
}
template<class Type>
inline const Type& SSMAX(const Type& arg1, const Type& arg2)
{
	return arg2 > arg1 ? arg2 : arg1;
}

// If they have not #included W32Base.h (part of my W32 utility library) then
// we need to define some stuff.  Otherwise, this is all defined there.

#if !defined(W32BASE_H)

	// If they want us to use only standard C++ stuff (no Win32 stuff)

	#ifdef SS_ANSI

		// On non-Win32 platforms, there is no TCHAR.H so define what we need

		#ifndef _WIN32

			typedef const char*		PCSTR;
			typedef char*			PSTR;
			typedef const wchar_t*	PCWSTR;
			typedef wchar_t*		PWSTR;
			#ifdef UNICODE
				typedef wchar_t		TCHAR;
			#else
				typedef char		TCHAR;
			#endif
			typedef wchar_t			OLECHAR;

		#else

			#include <TCHAR.H>
			#include <WTYPES.H>
			#ifndef STRICT
				#define STRICT
			#endif

		#endif	// #ifndef _WIN32


		// Make sure ASSERT and verify are defined in an ANSI fashion

		#ifndef ASSERT
			#include <assert.h>
			#define ASSERT(f) assert((f))
		#endif
		#ifndef VERIFY
			#ifdef _DEBUG
				#define VERIFY(x) ASSERT((x))
			#else
				#define VERIFY(x) x
			#endif
		#endif

	#else // #ifdef SS_ANSI

		#include <TCHAR.H>
		#include <WTYPES.H>
		#ifndef STRICT
			#define STRICT
		#endif

		// Make sure ASSERT and verify are defined

		#ifndef ASSERT
			#include <crtdbg.h>
			#define ASSERT(f) _ASSERTE((f))
		#endif
		#ifndef VERIFY
			#ifdef _DEBUG
				#define VERIFY(x) ASSERT((x))
			#else
				#define VERIFY(x) x
			#endif
		#endif

	#endif // #ifdef SS_ANSI

	#ifndef UNUSED
		#define UNUSED(x) x
	#endif

#endif // #ifndef W32BASE_H

// Standard headers needed

#include <string>			// basic_string
#include <algorithm>		// for_each, etc.
#include <functional>		// for StdStringLessNoCase, et al
#include <locale>			// for various facets

// If this is a recent enough version of VC include comdef.h, so we can write
// member functions to deal with COM types & compiler support classes e.g. _bstr_t

#if defined (_MSC_VER) && (_MSC_VER >= 1100)
	#include <comdef.h>
	#define SS_INC_COMDEF		// signal that we #included MS comdef.h file
	#define STDSTRING_INC_COMDEF
	#define SS_NOTHROW __declspec(nothrow)
#else
	#define SS_NOTHROW
#endif

#ifndef TRACE
	#define TRACE_DEFINED_HERE
	#define TRACE
#endif

// Microsoft defines PCSTR, PCWSTR, etc, but no PCTSTR.  I hate to use the
// versions with the "L" in front of them because that's a leftover from Win 16
// days, even though it evaluates to the same thing.  Therefore, Define a PCSTR
// as an LPCTSTR.

#if !defined(PCTSTR) && !defined(PCTSTR_DEFINED)
	typedef const TCHAR*			PCTSTR;
	#define PCTSTR_DEFINED
#endif

#if !defined(PCOLESTR) && !defined(PCOLESTR_DEFINED)
	typedef const OLECHAR*			PCOLESTR;
	#define PCOLESTR_DEFINED
#endif

#if !defined(POLESTR) && !defined(POLESTR_DEFINED)
	typedef OLECHAR*				POLESTR;
	#define POLESTR_DEFINED
#endif

#if !defined(PCUSTR) && !defined(PCUSTR_DEFINED)
	typedef const unsigned char*	PCUSTR;
	typedef unsigned char*			PUSTR;
	#define PCUSTR_DEFINED
#endif

// SS_USE_FACET macro and why we need it:
//
// Since I'm a good little Standard C++ programmer, I use locales.  Thus, I
// need to make use of the use_facet<> template function here.   Unfortunately,
// this need is complicated by the fact the MS' implementation of the Standard
// C++ Library has a non-standard version of use_facet that takes more
// arguments than the standard dictates.  Since I'm trying to write CStdString
// to work with any version of the Standard library, this presents a problem.
//
// The upshot of this is that I can't do 'use_facet' directly.  The MS' docs
// tell me that I have to use a macro, _USE() instead.  Since _USE obviously
// won't be available in other implementations, this means that I have to write
// my OWN macro -- SS_USE_FACET -- that evaluates either to _USE or to the
// standard, use_facet.
//
// If you are having trouble with the SS_USE_FACET macro, in your implementation
// of the Standard C++ Library, you can define your own version of SS_USE_FACET.
#ifndef schMSG
	#define schSTR(x)	   #x
	#define schSTR2(x)	schSTR(x)
	#define schMSG(desc) message(__FILE__ "(" schSTR2(__LINE__) "):" #desc)
#endif

#ifndef SS_USE_FACET
	// STLPort #defines a macro (__STL_NO_EXPLICIT_FUNCTION_TMPL_ARGS) for
	// all MSVC builds, erroneously in my opinion.  It causes problems for
	// my SS_ANSI builds.  In my code, I always comment out that line.  You'll
	// find it in   \stlport\config\stl_msvc.h
	#if defined(__SGI_STL_PORT) && (__SGI_STL_PORT >= 0x400 )
		#if defined(__STL_NO_EXPLICIT_FUNCTION_TMPL_ARGS) && defined(_MSC_VER)
			#ifdef SS_ANSI
				#pragma schMSG(__STL_NO_EXPLICIT_FUNCTION_TMPL_ARGS defined!!)
			#endif
		#endif
		#define SS_USE_FACET(loc, fac) std::use_facet<fac >(loc)
	#elif defined(_MSC_VER )
		#define SS_USE_FACET(loc, fac) _USE(loc, fac)

	// ...and
	#elif defined(_RWSTD_NO_TEMPLATE_ON_RETURN_TYPE)
        #define SS_USE_FACET(loc, fac) std::use_facet(loc, (fac*)0)
	#else
		#define SS_USE_FACET(loc, fac) std::use_facet<fac >(loc)
	#endif
#endif

// =============================================================================
// UNICODE/MBCS conversion macros.  Made to work just like the MFC/ATL ones.
// =============================================================================

// First define the conversion helper functions.  We define these regardless of
// any preprocessor macro settings since their names won't collide. 

#ifdef SS_ANSI // Are we doing things the standard, non-Win32 way?...

	typedef std::codecvt<wchar_t, char, mbstate_t> SSCodeCvt;

	// Not sure if we need all these headers.   I believe ANSI says we do.

	#include <stdio.h>
	#include <stdarg.h>
	#include <wchar.h>
	#ifndef va_start
		#include <varargs.h>
	#endif

	// StdCodeCvt - made to look like Win32 functions WideCharToMultiByte annd
	//              MultiByteToWideChar but uses locales in SS_ANSI builds
	inline PWSTR StdCodeCvt(PWSTR pW, PCSTR pA, int nChars,
		const std::locale& loc=std::locale())
	{
		ASSERT(0 != pA);
		ASSERT(0 != pW);
		pW[0] = '\0';
		PCSTR pBadA				= 0;
		PWSTR pBadW				= 0;
		SSCodeCvt::result res	= SSCodeCvt::ok;
		const SSCodeCvt& conv	= SS_USE_FACET(loc, SSCodeCvt);
        SSCodeCvt::state_type st= { 0 };
		res						= conv.in(st,
										  pA, pA + nChars, pBadA,
										  pW, pW + nChars, pBadW);
		ASSERT(SSCodeCvt::ok == res);
		return pW;
	}
	inline PWSTR StdCodeCvt(PWSTR pW, PCUSTR pA, int nChars,
		const std::locale& loc=std::locale())
	{
		return StdCodeCvt(pW, (PCSTR)pA, nChars, loc);
	}

	inline PSTR StdCodeCvt(PSTR pA, PCWSTR pW, int nChars,
		const std::locale& loc=std::locale())
	{
		ASSERT(0 != pA);
		ASSERT(0 != pW);
		pA[0] = '\0';
		PSTR pBadA				= 0;
		PCWSTR pBadW			= 0;
		SSCodeCvt::result res	= SSCodeCvt::ok;
		const SSCodeCvt& conv	= SS_USE_FACET(loc, SSCodeCvt);
        SSCodeCvt::state_type st= { 0 };
		res						= conv.out(st,
										   pW, pW + nChars, pBadW,
										   pA, pA + nChars, pBadA);
		ASSERT(SSCodeCvt::ok == res);
		return pA;
	}
	inline PUSTR StdCodeCvt(PUSTR pA, PCWSTR pW, int nChars,
		const std::locale& loc=std::locale())
	{
		return (PUSTR)StdCodeCvt((PSTR)pA, pW, nChars, loc);
	}

#else   // ...or are we doing things assuming win32 and Visual C++?

	#include <malloc.h>	// needed for _alloca

	inline PWSTR StdCodeCvt(PWSTR pW, PCSTR pA, int nChars, UINT acp=CP_ACP)
	{
		ASSERT(0 != pA);
		ASSERT(0 != pW);
		pW[0] = '\0';
		MultiByteToWideChar(acp, 0, pA, -1, pW, nChars);
		return pW;
	}
	inline PWSTR StdCodeCvt(PWSTR pW, PCUSTR pA, int nChars, UINT acp=CP_ACP)
	{
		return StdCodeCvt(pW, (PCSTR)pA, nChars, acp);
	}

	inline PSTR StdCodeCvt(PSTR pA, PCWSTR pW, int nChars, UINT acp=CP_ACP)
	{
		ASSERT(0 != pA);
		ASSERT(0 != pW);
		pA[0] = '\0';
		WideCharToMultiByte(acp, 0, pW, -1, pA, nChars, 0, 0);
		return pA;
	}
	inline PUSTR StdCodeCvt(PUSTR pA, PCWSTR pW, int nChars, UINT acp=CP_ACP)
	{
		return (PUSTR)StdCodeCvt((PSTR)pA, pW, nChars, acp);
	}

	// Define our conversion macros to look exactly like Microsoft's to
	// facilitate using this stuff both with and without MFC/ATL

	#ifdef _CONVERSION_USES_THREAD_LOCALE
		#ifndef _DEBUG
			#define SSCVT int _cvt; _cvt; UINT _acp=GetACP(); \
				_acp; PCWSTR _pw; _pw; PCSTR _pa; _pa
		#else
			#define SSCVT int _cvt = 0; _cvt; UINT _acp=GetACP();\
				 _acp; PCWSTR _pw=0; _pw; PCSTR _pa=0; _pa
		#endif
	#else
		#ifndef _DEBUG
			#define SSCVT int _cvt; _cvt; UINT _acp=CP_ACP; _acp;\
				 PCWSTR _pw; _pw; PCSTR _pa; _pa
		#else
			#define SSCVT int _cvt = 0; _cvt; UINT _acp=CP_ACP; \
				_acp; PCWSTR _pw=0; _pw; PCSTR _pa=0; _pa
		#endif
	#endif

	#ifdef _CONVERSION_USES_THREAD_LOCALE
		#define SSA2W(pa) (\
			((_pa = pa) == 0) ? 0 : (\
				_cvt = (strlen(_pa)+1),\
				StdCodeCvt((PWSTR) _alloca(_cvt*2), _pa, _cvt, _acp)))
		#define SSW2A(pw) (\
			((_pw = pw) == 0) ? 0 : (\
				_cvt = (wcslen(_pw)+1)*2,\
				StdW2AHelper((LPSTR) _alloca(_cvt), _pw, _cvt, _acp)))
	#else
		#define SSA2W(pa) (\
			((_pa = pa) == 0) ? 0 : (\
				_cvt = (strlen(_pa)+1),\
				StdCodeCvt((PWSTR) _alloca(_cvt*2), _pa, _cvt)))
		#define SSW2A(pw) (\
			((_pw = pw) == 0) ? 0 : (\
				_cvt = (wcslen(_pw)+1)*2,\
				StdCodeCvt((LPSTR) _alloca(_cvt), _pw, _cvt)))
	#endif

	#define SSA2CW(pa) ((PCWSTR)SSA2W((pa)))
	#define SSW2CA(pw) ((PCSTR)SSW2A((pw)))

	#ifdef UNICODE
		#define SST2A	SSW2A
		#define SSA2T	SSA2W
		#define SST2CA	SSW2CA
		#define SSA2CT	SSA2CW
		inline PWSTR	SST2W(PTSTR p)			{ return p; }
		inline PTSTR	SSW2T(PWSTR p)			{ return p; }
		inline PCWSTR	SST2CW(PCTSTR p)		{ return p; }
		inline PCTSTR	SSW2CT(PCWSTR p)		{ return p; }
	#else
		#define SST2W	SSA2W
		#define SSW2T	SSW2A
		#define SST2CW	SSA2CW
		#define SSW2CT	SSW2CA
		inline PSTR		SST2A(PTSTR p)			{ return p; }
		inline PTSTR	SSA2T(PSTR p)			{ return p; }
		inline PCSTR	SST2CA(PCTSTR p)		{ return p; }
		inline PCTSTR	SSA2CT(PCSTR p)			{ return p; }
	#endif // #ifdef UNICODE

	#if defined(UNICODE)
	// in these cases the default (TCHAR) is the same as OLECHAR
		inline PCOLESTR	SST2COLE(PCTSTR p)		{ return p; }
		inline PCTSTR	SSOLE2CT(PCOLESTR p)	{ return p; }
		inline POLESTR	SST2OLE(PTSTR p)		{ return p; }
		inline PTSTR	SSOLE2T(POLESTR p)		{ return p; }
	#elif defined(OLE2ANSI)
	// in these cases the default (TCHAR) is the same as OLECHAR
		inline PCOLESTR	SST2COLE(PCTSTR p)		{ return p; }
		inline PCTSTR	SSOLE2CT(PCOLESTR p)	{ return p; }
		inline POLESTR	SST2OLE(PTSTR p)		{ return p; }
		inline PTSTR	SSOLE2T(POLESTR p)		{ return p; }
	#else
		//CharNextW doesn't work on Win95 so we use this
		#define SST2COLE(pa)	SSA2CW((pa))
		#define SST2OLE(pa)		SSA2W((pa))
		#define SSOLE2CT(po)	SSW2CA((po))
		#define SSOLE2T(po)		SSW2A((po))
	#endif

	#ifdef OLE2ANSI
		#define SSW2OLE		SSW2A
		#define SSOLE2W		SSA2W
		#define SSW2COLE	SSW2CA
		#define SSOLE2CW	SSA2CW
		inline POLESTR		SSA2OLE(PSTR p)		{ return p; }
		inline PSTR			SSOLE2A(POLESTR p)	{ return p; }
		inline PCOLESTR		SSA2COLE(PCSTR p)	{ return p; }
		inline PCSTR		SSOLE2CA(PCOLESTR p){ return p; }
	#else
		#define SSA2OLE		SSA2W
		#define SSOLE2A		SSW2A
		#define SSA2COLE	SSA2CW
		#define SSOLE2CA	SSW2CA
		inline POLESTR		SSW2OLE(PWSTR p)	{ return p; }
		inline PWSTR		SSOLE2W(POLESTR p)	{ return p; }
		inline PCOLESTR		SSW2COLE(PCWSTR p)	{ return p; }
		inline PCWSTR		SSOLE2CW(PCOLESTR p){ return p; }
	#endif

	// Above we've defined macros that look like MS' but all have
	// an 'SS' prefix.  Now we need the real macros.  We'll either
	// get them from the macros above or from MFC/ATL.  If
	// SS_NO_CONVERSION is #defined, we'll forgo them

	#ifndef SS_NO_CONVERSION

		#if defined (USES_CONVERSION)

			#define _NO_STDCONVERSION	// just to be consistent

		#else

			#ifdef _MFC_VER

				#include <afxconv.h>
				#define _NO_STDCONVERSION // just to be consistent

			#else

				#define USES_CONVERSION SSCVT
				#define A2CW			SSA2CW
				#define W2CA			SSW2CA
				#define T2A				SST2A
				#define A2T				SSA2T
				#define T2W				SST2W
				#define W2T				SSW2T
				#define T2CA			SST2CA
				#define A2CT			SSA2CT
				#define T2CW			SST2CW
				#define W2CT			SSW2CT
				#define ocslen			sslen
				#define ocscpy			sscpy
				#define T2COLE			SST2COLE
				#define OLE2CT			SSOLE2CT
				#define T2OLE			SST2COLE
				#define OLE2T			SSOLE2CT
				#define A2OLE			SSA2OLE
				#define OLE2A			SSOLE2A
				#define W2OLE			SSW2OLE
				#define OLE2W			SSOLE2W
				#define A2COLE			SSA2COLE
				#define OLE2CA			SSOLE2CA
				#define W2COLE			SSW2COLE
				#define OLE2CW			SSOLE2CW
		
			#endif // #ifdef _MFC_VER
		#endif // #ifndef USES_CONVERSION
	#endif // #ifndef SS_NO_CONVERSION

	// Define ostring - generic name for std::basic_string<OLECHAR>

	#if !defined(ostring) && !defined(OSTRING_DEFINED)
		typedef std::basic_string<OLECHAR> ostring;
		#define OSTRING_DEFINED
	#endif

#endif // #ifndef SS_ANSI

// StdCodeCvt when there's no conversion to be done
inline PSTR StdCodeCvt(PSTR pDst, PCSTR pSrc, int nChars)
{
	pDst[0]				= '\0';
	std::char_traits<char>::copy(pDst, pSrc, nChars);
	if ( nChars > 0 )
		pDst[nChars]	= '\0';

	return pDst;
}
inline PSTR StdCodeCvt(PSTR pDst, PCUSTR pSrc, int nChars)
{
	return StdCodeCvt(pDst, (PCSTR)pSrc, nChars);
}
inline PUSTR StdCodeCvt(PUSTR pDst, PCSTR pSrc, int nChars)
{
	return (PUSTR)StdCodeCvt((PSTR)pDst, pSrc, nChars);
}

inline PWSTR StdCodeCvt(PWSTR pDst, PCWSTR pSrc, int nChars)
{
	pDst[0]				= '\0';
	std::char_traits<wchar_t>::copy(pDst, pSrc, nChars);
	if ( nChars > 0 )
		pDst[nChars]	= '\0';

	return pDst;
}


// Define tstring -- generic name for std::basic_string<TCHAR>

#if !defined(tstring) && !defined(TSTRING_DEFINED)
	typedef std::basic_string<TCHAR> tstring;
	#define TSTRING_DEFINED
#endif

// a very shorthand way of applying the fix for KB problem Q172398
// (basic_string assignment bug)

#if defined ( _MSC_VER ) && ( _MSC_VER < 1200 )
	#define Q172398(x) (x).erase()
#else
	#define Q172398(x)
#endif

// =============================================================================
// INLINE FUNCTIONS ON WHICH CSTDSTRING RELIES
//
// Usually for generic text mapping, we rely on preprocessor macro definitions
// to map to string functions.  However the CStdStr<> template cannot use
// macro-based generic text mappings because its character types do not get
// resolved until template processing which comes AFTER macro processing.  In
// other words, UNICODE is of little help to us in the CStdStr template
//
// Therefore, to keep the CStdStr declaration simple, we have these inline
// functions.  The template calls them often.  Since they are inline (and NOT
// exported when this is built as a DLL), they will probably be resolved away
// to nothing. 
//
// Without these functions, the CStdStr<> template would probably have to broken
// out into two, almost identical classes.  Either that or it would be a huge,
// convoluted mess, with tons of "if" statements all over the place checking the
// size of template parameter CT.
// 
// In several cases, you will see two versions of each function.  One version is
// the more portable, standard way of doing things, while the other is the
// non-standard, but often significantly faster Visual C++ way.
// =============================================================================

// If they defined SS_NO_REFCOUNT, then we must convert all assignments

#ifdef SS_NO_REFCOUNT
	#define SSREF(x) (x).c_str()
#else
	#define SSREF(x) (x)
#endif

// -----------------------------------------------------------------------------
// sslen: strlen/wcslen wrappers
// -----------------------------------------------------------------------------
template<typename CT> inline int sslen(const CT* pT)
{
	return 0 == pT ? 0 : std::char_traits<CT>::length(pT);
}
inline SS_NOTHROW int sslen(const std::string& s)
{
	return s.length();
}
inline SS_NOTHROW int sslen(const std::wstring& s)
{
	return s.length();
}


// -----------------------------------------------------------------------------
// ssasn: assignment functions -- assign "sSrc" to "sDst"
// -----------------------------------------------------------------------------
typedef std::string::size_type		SS_SIZETYPE; // just for shorthand, really
typedef std::string::pointer		SS_PTRTYPE;  
typedef std::wstring::size_type		SW_SIZETYPE;
typedef std::wstring::pointer		SW_PTRTYPE;  

inline void	ssasn(std::string& sDst, const std::string& sSrc)
{
	if ( sDst.c_str() != sSrc.c_str() )
	{
		sDst.erase();
		sDst.assign(SSREF(sSrc));
	}
}
inline void	ssasn(std::string& sDst, PCSTR pA)
{
	// Watch out for NULLs, as always.

	if ( 0 == pA )
	{
		sDst.erase();
	}

	// If pA actually points to part of sDst, we must NOT erase(), but
	// rather take a substring

	else if ( pA >= sDst.c_str() && pA <= sDst.c_str() + sDst.size() )
	{
		sDst =sDst.substr(static_cast<SS_SIZETYPE>(pA-sDst.c_str()));
	}

	// Otherwise (most cases) apply the assignment bug fix, if applicable
	// and do the assignment

	else
	{
		Q172398(sDst);
		sDst.assign(pA);
	}
}
inline void	ssasn(std::string& sDst, const std::wstring& sSrc)
{
#ifdef SS_ANSI
	int nLen	= sSrc.size();
	sDst.resize(0);
	sDst.resize(nLen);
	StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()), sSrc.c_str(), nLen);
#else
	SSCVT;
	sDst.assign(SSW2CA(sSrc.c_str()));
#endif
}
inline void	ssasn(std::string& sDst, PCWSTR pW)
{
#ifdef SS_ANSI
	int nLen	= sslen(pW);
	sDst.resize(0);
	sDst.resize(nLen);
	StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()), pW, nLen);
#else
	SSCVT;
	sDst.assign(pW ? SSW2CA(pW) : "");
#endif
}
inline void ssasn(std::string& sDst, const int nNull)
{
	UNUSED(nNull);
	ASSERT(nNull==0);
	sDst.assign("");
}	
inline void	ssasn(std::wstring& sDst, const std::wstring& sSrc)
{
	if ( sDst.c_str() != sSrc.c_str() )
	{
		sDst.erase();
		sDst.assign(SSREF(sSrc));
	}
}
inline void	ssasn(std::wstring& sDst, PCWSTR pW)
{
	// Watch out for NULLs, as always.

	if ( 0 == pW )
	{
		sDst.erase();
	}

	// If pW actually points to part of sDst, we must NOT erase(), but
	// rather take a substring

	else if ( pW >= sDst.c_str() && pW <= sDst.c_str() + sDst.size() )
	{
		sDst = sDst.substr(static_cast<SW_SIZETYPE>(pW-sDst.c_str()));
	}

	// Otherwise (most cases) apply the assignment bug fix, if applicable
	// and do the assignment

	else
	{
		Q172398(sDst);
		sDst.assign(pW);
	}
}
#undef StrSizeType
inline void	ssasn(std::wstring& sDst, const std::string& sSrc)
{
#ifdef SS_ANSI
	int nLen	= sSrc.size();
	sDst.resize(0);
	sDst.resize(nLen);
	StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()), sSrc.c_str(), nLen);
#else
	SSCVT;
	sDst.assign(SSA2CW(sSrc.c_str()));
#endif
}
inline void	ssasn(std::wstring& sDst, PCSTR pA)
{
#ifdef SS_ANSI
	int nLen	= sslen(pA);
	sDst.resize(0);
	sDst.resize(nLen);
	StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()), pA, nLen);
#else
	SSCVT;
	sDst.assign(pA ? SSA2CW(pA) : L"");
#endif
}
inline void ssasn(std::wstring& sDst, const int nNull)
{
	UNUSED(nNull);
	ASSERT(nNull==0);
	sDst.assign(L"");
}


// -----------------------------------------------------------------------------
// ssadd: string object concatenation -- add second argument to first
// -----------------------------------------------------------------------------
inline void	ssadd(std::string& sDst, const std::wstring& sSrc)
{
#ifdef SS_ANSI
	int nLen	= sSrc.size();
	sDst.resize(sDst.size() + nLen);
	StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()+nLen), sSrc.c_str(), nLen);
#else
	SSCVT; 
	sDst.append(SSW2CA(sSrc.c_str())); 
#endif
}
inline void	ssadd(std::string& sDst, const std::string& sSrc)
{ 
	sDst.append(sSrc.c_str());
}
inline void	ssadd(std::string& sDst, PCWSTR pW)
{
#ifdef SS_ANSI
	int nLen	= sslen(pW);
	sDst.resize(sDst.size() + nLen);
	StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()+nLen), pW, nLen);
#else
	SSCVT;
	if ( 0 != pW )
		sDst.append(SSW2CA(pW)); 
#endif
}
inline void	ssadd(std::string& sDst, PCSTR pA)
{
	if ( pA )
		sDst.append(pA); 
}
inline void	ssadd(std::wstring& sDst, const std::wstring& sSrc)
{
	sDst.append(sSrc.c_str());
}
inline void	ssadd(std::wstring& sDst, const std::string& sSrc)
{
#ifdef SS_ANSI
	int nLen	= sSrc.size();
	sDst.resize(sDst.size() + nLen);
	StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()+nLen), sSrc.c_str(), nLen);
#else
	SSCVT;
	sDst.append(SSA2CW(sSrc.c_str()));
#endif
}
inline void	ssadd(std::wstring& sDst, PCSTR pA)
{
#ifdef SS_ANSI
	int nLen	= sslen(pA);
	sDst.resize(sDst.size() + nLen);
	StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()+nLen), pA, nLen);
#else
	SSCVT;
	if ( 0 != pA )
		sDst.append(SSA2CW(pA));
#endif
}
inline void	ssadd(std::wstring& sDst, PCWSTR pW)
{
	if ( pW )
		sDst.append(pW);
}


// -----------------------------------------------------------------------------
// ssicmp: comparison (case insensitive )
// -----------------------------------------------------------------------------
#ifdef SS_ANSI
	template<typename CT>
	inline int ssicmp(const CT* pA1, const CT* pA2)
	{
		std::locale loc;
		const std::ctype<CT>& ct = SS_USE_FACET(loc, std::ctype<CT>);
		CT f;
		CT l;

            do 
			{
				f = ct.tolower(*(pA1++));
				l = ct.tolower(*(pA2++));
            } while ( (f) && (f == l) );

        return (int)(f - l);
	}
#else
	#ifdef _MBCS
		inline long sscmp(PCSTR pA1, PCSTR pA2)
		{
			return _mbscmp((PCUSTR)pA1, (PCUSTR)pA2);
		}
		inline long ssicmp(PCSTR pA1, PCSTR pA2)
		{
			return _mbsicmp((PCUSTR)pA1, (PCUSTR)pA2);
		}
	#else
		inline long sscmp(PCSTR pA1, PCSTR pA2)
		{
			return strcmp(pA1, pA2);
		}
		inline long ssicmp(PCSTR pA1, PCSTR pA2)
		{
			return _stricmp(pA1, pA2);
		}
	#endif
	inline long sscmp(PCWSTR pW1, PCWSTR pW2)
	{
		return wcscmp(pW1, pW2);
	}
	inline long ssicmp(PCWSTR pW1, PCWSTR pW2)
	{
		return _wcsicmp(pW1, pW2);
	}
#endif

// -----------------------------------------------------------------------------
// ssupr/sslwr: Uppercase/Lowercase conversion functions
// -----------------------------------------------------------------------------
#ifdef SS_ANSI
	template<typename CT>
	inline void sslwr(CT* pT, size_t nLen)
	{
		SS_USE_FACET(std::locale(), std::ctype<CT>).tolower(pT, pT+nLen);
	}
	template<typename CT>
	inline void ssupr(CT* pT, size_t nLen)
	{
		SS_USE_FACET(std::locale(), std::ctype<CT>).toupper(pT, pT+nLen);
	}
#else  // #else we must be on Win32
	#ifdef _MBCS
		inline void	ssupr(PSTR pA, size_t /*nLen*/)
		{
			_mbsupr((PUSTR)pA);
		}
		inline void	sslwr(PSTR pA, size_t /*nLen*/)
		{
			_mbslwr((PUSTR)pA);
		}
	#else
		inline void	ssupr(PSTR pA, size_t /*nLen*/)
		{
			_strupr(pA); 
		}
		inline void	sslwr(PSTR pA, size_t /*nLen*/)
		{
			_strlwr(pA);
		}
	#endif
	inline void	ssupr(PWSTR pW, size_t /*nLen*/)	
	{
		_wcsupr(pW);
	}
	inline void	sslwr(PWSTR pW, size_t /*nLen*/)	
	{
		_wcslwr(pW);
	}
#endif // #ifdef SS_ANSI

// -----------------------------------------------------------------------------
//  vsprintf/vswprintf or _vsnprintf/_vsnwprintf equivalents.  In standard
//  builds we can't use _vsnprintf/_vsnwsprintf because they're MS extensions.
// -----------------------------------------------------------------------------
#ifdef SS_ANSI
	inline int ssvsprintf(PSTR pA, size_t /*nCount*/, PCSTR pFmtA, va_list vl)
	{
		return vsprintf(pA, pFmtA, vl);
	}
	inline int ssvsprintf(PWSTR pW, size_t nCount, PCWSTR pFmtW, va_list vl)
	{
		// JMO: It is beginning to seem like Microsoft Visual C++ is the only
		// CRT distribution whose version of vswprintf takes THREE arguments.
		// All others seem to take FOUR arguments.  Therefore instead of 
		// checking for every possible distro here, I'll assume that unless
		// I am running with Microsoft's CRT, then vswprintf takes four
		// arguments.  If you get a compilation error here, then you can just
		// change this code to call the three-argument version.
//	#if !defined(__MWERKS__) && !defined(__SUNPRO_CC_COMPAT) && !defined(__SUNPRO_CC)
	#ifndef _MSC_VER
		return vswprintf(pW, nCount, pFmtW, vl);
	#else
		nCount;
		return vswprintf(pW, pFmtW, vl);
	#endif
	}
#else
	inline int	ssnprintf(PSTR pA, size_t nCount, PCSTR pFmtA, va_list vl)
	{ 
		return _vsnprintf(pA, nCount, pFmtA, vl);
	}
	inline int	ssnprintf(PWSTR pW, size_t nCount, PCWSTR pFmtW, va_list vl)
	{
		return _vsnwprintf(pW, nCount, pFmtW, vl);
	}
#endif


// -----------------------------------------------------------------------------
// ssload: Type safe, overloaded ::LoadString wrappers
// There is no equivalent of these in non-Win32-specific builds.  However, I'm
// thinking that with the message facet, there might eventually be one
// -----------------------------------------------------------------------------
#ifdef SS_ANSI
#else
	inline int ssload(HMODULE hInst, UINT uId, PSTR pBuf, int nMax)
	{
		return ::LoadStringA(hInst, uId, pBuf, nMax);
	}
	inline int ssload(HMODULE hInst, UINT uId, PWSTR pBuf, int nMax)
	{
		return ::LoadStringW(hInst, uId, pBuf, nMax);
	}
#endif


// -----------------------------------------------------------------------------
// sscoll/ssicoll: Collation wrappers
// -----------------------------------------------------------------------------
#ifdef SS_ANSI
	template <typename CT>
	inline int sscoll(const CT* sz1, int nLen1, const CT* sz2, int nLen2)
	{
		const std::collate<CT>& coll =
			SS_USE_FACET(std::locale(), std::collate<CT>);
		return coll.compare(sz1, sz1+nLen1, sz2, sz2+nLen2);
	}
	template <typename CT>
	inline int ssicoll(const CT* sz1, int nLen1, const CT* sz2, int nLen2)
	{
		const std::locale loc;
		const std::collate<CT>& coll = SS_USE_FACET(loc, std::collate<CT>);

		// Some implementations seem to have trouble using the collate<>
		// facet typedefs so we'll just default to basic_string and hope
		// that's what the collate facet uses (which it generally should)

//		std::collate<CT>::string_type s1(sz1);
//		std::collate<CT>::string_type s2(sz2);
		std::basic_string<CT> s1(sz1);
		std::basic_string<CT> s2(sz2);

		sslwr(const_cast<CT*>(s1.c_str()), nLen1);
		sslwr(const_cast<CT*>(s2.c_str()), nLen2);
		return coll.compare(s1.c_str(), s1.c_str()+nLen1,
							s2.c_str(), s2.c_str()+nLen2);
	}
#else
	#ifdef _MBCS
		inline int sscoll(PCSTR sz1, int /*nLen1*/, PCSTR sz2, int /*nLen2*/)
		{
			return _mbscoll((PCUSTR)sz1, (PCUSTR)sz2);
		}
		inline int ssicoll(PCSTR sz1, int /*nLen1*/, PCSTR sz2, int /*nLen2*/)
		{
			return _mbsicoll((PCUSTR)sz1, (PCUSTR)sz2);
		}
	#else
		inline int sscoll(PCSTR sz1, int /*nLen1*/, PCSTR sz2, int /*nLen2*/)
		{
			return strcoll(sz1, sz2);
		}
		inline int ssicoll(PCSTR sz1, int /*nLen1*/, PCSTR sz2, int /*nLen2*/)
		{
			return _stricoll(sz1, sz2);
		}
	#endif
	inline int sscoll(PCWSTR sz1, int /*nLen1*/, PCWSTR sz2, int /*nLen2*/)
	{
		return wcscoll(sz1, sz2);
	}
	inline int ssicoll(PCWSTR sz1, int /*nLen1*/, PCWSTR sz2, int /*nLen2*/)
	{
		return _wcsicoll(sz1, sz2);
	}
#endif


// -----------------------------------------------------------------------------
// ssfmtmsg: FormatMessage equivalents.  Needed because I added a CString facade
// Again -- no equivalent of these on non-Win32 builds but their might one day
// be one if the message facet gets implemented
// -----------------------------------------------------------------------------
#ifdef SS_ANSI
#else
	inline DWORD ssfmtmsg(DWORD dwFlags, LPCVOID pSrc, DWORD dwMsgId,
						  DWORD dwLangId, PSTR pBuf, DWORD nSize,
						  va_list* vlArgs)
	{ 
		return FormatMessageA(dwFlags, pSrc, dwMsgId, dwLangId,
							  pBuf, nSize,vlArgs);
	}
	inline DWORD ssfmtmsg(DWORD dwFlags, LPCVOID pSrc, DWORD dwMsgId,
						  DWORD dwLangId, PWSTR pBuf, DWORD nSize,
						  va_list* vlArgs)
	{
		return FormatMessageW(dwFlags, pSrc, dwMsgId, dwLangId,
							  pBuf, nSize,vlArgs);
	}
#endif
 


// FUNCTION: sscpy.  Copies up to 'nMax' characters from pSrc to pDst.
// -----------------------------------------------------------------------------
// FUNCTION:  sscpy
//		inline int sscpy(PSTR pDst, PCSTR pSrc, int nMax=-1);
//		inline int sscpy(PUSTR pDst,  PCSTR pSrc, int nMax=-1)
//		inline int sscpy(PSTR pDst, PCWSTR pSrc, int nMax=-1);
//		inline int sscpy(PWSTR pDst, PCWSTR pSrc, int nMax=-1);
//		inline int sscpy(PWSTR pDst, PCSTR pSrc, int nMax=-1);
//
// DESCRIPTION:
//		This function is very much (but not exactly) like strcpy.  These
//		overloads simplify copying one C-style string into another by allowing
//		the caller to specify two different types of strings if necessary.
//
//		The strings must NOT overlap
//
//		"Character" is expressed in terms of the destination string, not
//		the source.  If no 'nMax' argument is supplied, then the number of
//		characters copied will be sslen(pSrc).  A NULL terminator will
//		also be added so pDst must actually be big enough to hold nMax+1
//		characters.  The return value is the number of characters copied,
//		not including the NULL terminator.
//
// PARAMETERS: 
//		pSrc - the string to be copied FROM.  May be a char based string, an
//			   MBCS string (in Win32 builds) or a wide string (wchar_t).
//		pSrc - the string to be copied TO.  Also may be either MBCS or wide
//		nMax - the maximum number of characters to be copied into szDest.  Note
//			   that this is expressed in whatever a "character" means to pDst.
//			   If pDst is a wchar_t type string than this will be the maximum
//			   number of wchar_ts that my be copied.  The pDst string must be
//			   large enough to hold least nMaxChars+1 characters.
//			   If the caller supplies no argument for nMax this is a signal to
//			   the routine to copy all the characters in pSrc, regardless of
//			   how long it is.
//
// RETURN VALUE: none
// -----------------------------------------------------------------------------
template<typename CT1, typename CT2>
inline int sscpycvt(CT1* pDst, const CT2* pSrc, int nChars)
{
	StdCodeCvt(pDst, pSrc, nChars);
	pDst[SSMAX(nChars, 0)]	= '\0';
	return nChars;
}

template<typename CT1, typename CT2>
inline int sscpy(CT1* pDst, const CT2* pSrc, int nMax, int nLen)
{
	return sscpycvt(pDst, pSrc, SSMIN(nMax, nLen));
}
template<typename CT1, typename CT2>
inline int sscpy(CT1* pDst, const CT2* pSrc, int nMax)
{
	return sscpycvt(pDst, pSrc, SSMIN(nMax, sslen(pSrc)));
}
template<typename CT1, typename CT2>
inline int sscpy(CT1* pDst, const CT2* pSrc)
{
	return sscpycvt(pDst, pSrc, sslen(pSrc));
}
template<typename CT1, typename CT2>
inline int sscpy(CT1* pDst, const std::basic_string<CT2>& sSrc, int nMax)
{
	return sscpycvt(pDst, sSrc.c_str(), SSMIN(nMax, (int)sSrc.length()));
}
template<typename CT1, typename CT2>
inline int sscpy(CT1* pDst, const std::basic_string<CT2>& sSrc)
{
	return sscpycvt(pDst, sSrc.c_str(), (int)sSrc.length());
}

#ifdef SS_INC_COMDEF
	template<typename CT1>
	inline int sscpy(CT1* pDst, const _bstr_t& bs, int nMax)
	{
		return sscpycvt(pDst, static_cast<PCOLESTR>(bs), SSMIN(nMax, (int)bs.length()));
	}
	template<typename CT1>
	inline int sscpy(CT1* pDst, const _bstr_t& bs)
	{
		return sscpy(pDst, bs, bs.length());
	}
#endif


// -----------------------------------------------------------------------------
// Functional objects for changing case.  They also let you pass locales
// -----------------------------------------------------------------------------

#ifdef SS_ANSI
	template<typename CT>
	struct SSToUpper : public std::binary_function<CT, std::locale, CT>
	{
		inline CT operator()(const CT& t, const std::locale& loc) const
		{
			return std::toupper<CT>(t, loc);
		}
	};
	template<typename CT>
	struct SSToLower : public std::binary_function<CT, std::locale, CT>
	{
		inline CT operator()(const CT& t, const std::locale& loc) const
		{
			return std::tolower<CT>(t, loc);
		}
	};
#endif

// This struct is used for TrimRight() and TrimLeft() function implementations.
//template<typename CT>
//struct NotSpace : public std::unary_function<CT, bool>
//{
//	const std::locale& loc;
//	inline NotSpace(const std::locale& locArg) : loc(locArg) {}
//	inline bool operator() (CT t) { return !std::isspace(t, loc); }
//};
template<typename CT>
struct NotSpace : public std::unary_function<CT, bool>
{
	const std::locale& loc;
	NotSpace(const std::locale& locArg) : loc(locArg) {}

	// DINKUMWARE BUG:
	// Note -- using std::isspace in a COM DLL gives us access violations
	// because it causes the dynamic addition of a function to be called
	// when the library shuts down.  Unfortunately the list is maintained
	// in DLL memory but the function is in static memory.  So the COM DLL
	// goes away along with the function that was supposed to be called,
	// and then later when the DLL CRT shuts down it unloads the list and
	// tries to call the long-gone function.
	// This is DinkumWare's implementation problem.  Until then, we will
	// use good old isspace and iswspace from the CRT unless they
	// specify SS_ANSI
#ifdef SS_ANSI
	bool operator() (CT t) const { return !std::isspace(t, loc); }
#else
	bool ssisp(char c) const { return FALSE != ::isspace((int) c); }
	bool ssisp(wchar_t c) const { return FALSE != ::iswspace((wint_t) c); }
	bool operator()(CT t) const  { return !ssisp(t); }
#endif
};




//			Now we can define the template (finally!)
// =============================================================================
// TEMPLATE: CStdStr
//		template<typename CT> class CStdStr : public std::basic_string<CT>
//
// REMARKS:
//		This template derives from basic_string<CT> and adds some MFC CString-
//		like functionality
//
//		Basically, this is my attempt to make Standard C++ library strings as
//		easy to use as the MFC CString class.
//
//		Note that although this is a template, it makes the assumption that the
//		template argument (CT, the character type) is either char or wchar_t.  
// =============================================================================

//#define CStdStr _SS	// avoid compiler warning 4786


template<typename CT>
class CStdStr : public std::basic_string<CT>
{
	// Typedefs for shorter names.  Using these names also appears to help
	// us avoid some ambiguities that otherwise arise on some platforms

	typedef typename std::basic_string<CT>		MYBASE;	 // my base class
	typedef CStdStr<CT>							MYTYPE;	 // myself
	typedef typename MYBASE::const_pointer		PCMYSTR; // PCSTR or PCWSTR 
	typedef typename MYBASE::pointer			PMYSTR;	 // PSTR or PWSTR
	typedef typename MYBASE::iterator			MYITER;  // my iterator type
	typedef typename MYBASE::const_iterator		MYCITER; // you get the idea...
	typedef typename MYBASE::reverse_iterator	MYRITER;
	typedef typename MYBASE::size_type			MYSIZE;   
	typedef typename MYBASE::value_type			MYVAL; 
	typedef typename MYBASE::allocator_type		MYALLOC;
	
public:

	// shorthand conversion from PCTSTR to string resource ID
	#define _TRES(pctstr) (LOWORD((DWORD)(pctstr)))	

	// CStdStr inline constructors
	CStdStr()
	{
	}

	CStdStr(const MYTYPE& str) : MYBASE(SSREF(str))
	{
	}

	CStdStr(const std::string& str)
	{
		ssasn(*this, SSREF(str));
	}

	CStdStr(const std::wstring& str)
	{
		ssasn(*this, SSREF(str));
	}

	CStdStr(PCMYSTR pT, MYSIZE n) : MYBASE(pT, n)
	{
	}

	CStdStr(PCSTR pA)
	{
	#ifdef SS_ANSI
		*this = pA;
	#else
		if ( 0 != HIWORD(pA) )
			*this = pA;
		else if ( 0 != pA && !Load(_TRES(pA)) )
			TRACE(_T("Can't load string %u\n"), _TRES(pA));
	#endif
	}

	CStdStr(PCWSTR pW)
	{
	#ifdef SS_ANSI
		*this = pW;
	#else
		if ( 0 != HIWORD(pW) )
			*this = pW;
		else if ( 0 != pW && !Load(_TRES(pW)) )
			TRACE(_T("Can't load string %u\n"), _TRES(pW));
	#endif
	}

	CStdStr(MYCITER first, MYCITER last)
		: MYBASE(first, last)
	{
	}

	CStdStr(MYSIZE nSize, MYVAL ch, const MYALLOC& al=MYALLOC())
		: MYBASE(nSize, ch, al)
	{
	}

	#ifdef SS_INC_COMDEF
		CStdStr(const _bstr_t& bstr)
		{
			if ( bstr.length() > 0 )
				append(static_cast<PCMYSTR>(bstr), bstr.length());
		}
	#endif

	// CStdStr inline assignment operators -- the ssasn function now takes care
	// of fixing  the MSVC assignment bug (see knowledge base article Q172398).
	MYTYPE& operator=(const MYTYPE& str)
	{ 
		ssasn(*this, str); 
		return *this;
	}

	MYTYPE& operator=(const std::string& str)
	{
		ssasn(*this, str);
		return *this;
	}

	MYTYPE& operator=(const std::wstring& str)
	{
		ssasn(*this, str);
		return *this;
	}

	MYTYPE& operator=(PCSTR pA)
	{
		ssasn(*this, pA);
		return *this;
	}

	MYTYPE& operator=(PCWSTR pW)
	{
		ssasn(*this, pW);
		return *this;
	}

	MYTYPE& operator=(CT t)
	{
		Q172398(*this);
		MYBASE::assign(1, t);
		return *this;
	}

	#ifdef SS_INC_COMDEF
		MYTYPE& operator=(const _bstr_t& bstr)
		{
			if ( bstr.length() > 0 )
				return assign(static_cast<PCMYSTR>(bstr), bstr.length());
			else
			{
				erase();
				return *this;
			}
		}
	#endif


	// Overloads  also needed to fix the MSVC assignment bug (KB: Q172398)
	//  *** Thanks to Pete The Plumber for catching this one ***
	// They also are compiled if you have explicitly turned off refcounting
	#if ( defined(_MSC_VER) && ( _MSC_VER < 1200 ) ) || defined(SS_NO_REFCOUNT) 

		MYTYPE& assign(const MYTYPE& str)
		{
			ssasn(*this, str);
			return *this;
		}

		MYTYPE& assign(const MYTYPE& str, MYSIZE nStart, MYSIZE nChars)
		{
			// This overload of basic_string::assign is supposed to assign up to
			// <nChars> or the NULL terminator, whichever comes first.  Since we
			// are about to call a less forgiving overload (in which <nChars>
			// must be a valid length), we must adjust the length here to a safe
			// value.  Thanks to Ullrich Pollähne for catching this bug

			nChars		= SSMIN(nChars, str.length() - nStart);

			// Watch out for assignment to self

			if ( this == &str )
			{
				MYTYPE strTemp(str.c_str()+nStart, nChars);
				assign(strTemp);
			}
			else
			{
				Q172398(*this);
				MYBASE::assign(str.c_str()+nStart, nChars);
			}
			return *this;
		}

		MYTYPE& assign(const MYBASE& str)
		{
			ssasn(*this, str);
			return *this;
		}

		MYTYPE& assign(const MYBASE& str, MYSIZE nStart, MYSIZE nChars)
		{
			// This overload of basic_string::assign is supposed to assign up to
			// <nChars> or the NULL terminator, whichever comes first.  Since we
			// are about to call a less forgiving overload (in which <nChars>
			// must be a valid length), we must adjust the length here to a safe
			// value. Thanks to Ullrich Pollähne for catching this bug

			nChars		= SSMIN(nChars, str.length() - nStart);

			// Watch out for assignment to self

			if ( this == &str )	// watch out for assignment to self
			{
				MYTYPE strTemp(str.c_str() + nStart, nChars);
				assign(strTemp);
			}
			else
			{
				Q172398(*this);
				MYBASE::assign(str.c_str()+nStart, nChars);
			}
			return *this;
		}

		MYTYPE& assign(const CT* pC, MYSIZE nChars)
		{
			// Q172398 only fix -- erase before assigning, but not if we're
			// assigning from our own buffer

	#if defined ( _MSC_VER ) && ( _MSC_VER < 1200 )
			if ( !empty() && ( pC < data() || pC > data() + capacity() ) )
				erase();
	#endif
			Q172398(*this);
			MYBASE::assign(pC, nChars);
			return *this;
		}

		MYTYPE& assign(MYSIZE nChars, MYVAL val)
		{
			Q172398(*this);
			MYBASE::assign(nChars, val);
			return *this;
		}

		MYTYPE& assign(const CT* pT)
		{
			return assign(pT, CStdStr::traits_type::length(pT));
		}

		MYTYPE& assign(MYCITER iterFirst, MYCITER iterLast)
		{
	#if defined ( _MSC_VER ) && ( _MSC_VER < 1200 ) 
			// Q172398 fix.  don't call erase() if we're assigning from ourself
			if ( iterFirst < begin() || iterFirst > begin() + size() )
				erase()
	#endif
			replace(begin(), end(), iterFirst, iterLast);
			return *this;
		}
	#endif


	// -------------------------------------------------------------------------
	// CStdStr inline concatenation.
	// -------------------------------------------------------------------------
	MYTYPE& operator+=(const MYTYPE& str)
	{
		ssadd(*this, str);
		return *this;
	}

	MYTYPE& operator+=(const std::string& str)
	{
		ssadd(*this, str);
		return *this; 
	}

	MYTYPE& operator+=(const std::wstring& str)
	{
		ssadd(*this, str);
		return *this;
	}

	MYTYPE& operator+=(PCSTR pA)
	{
		ssadd(*this, pA);
		return *this;
	}

	MYTYPE& operator+=(PCWSTR pW)
	{
		ssadd(*this, pW);
		return *this;
	}

	MYTYPE& operator+=(CT t)
	{
		append(1, t);
		return *this;
	}
	#ifdef SS_INC_COMDEF	// if we have _bstr_t, define a += for it too.
		MYTYPE& operator+=(const _bstr_t& bstr)
		{
			return operator+=(static_cast<PCMYSTR>(bstr));
		}
	#endif


	// addition operators -- global friend functions.

	friend	MYTYPE	operator+(const MYTYPE& str1,	const MYTYPE& str2);
	friend	MYTYPE	operator+(const MYTYPE& str,	CT t);
	friend	MYTYPE	operator+(const MYTYPE& str,	PCSTR sz);
	friend	MYTYPE	operator+(const MYTYPE& str,	PCWSTR sz);
	friend	MYTYPE	operator+(PCSTR pA,				const MYTYPE& str);
	friend	MYTYPE	operator+(PCWSTR pW,			const MYTYPE& str);
#ifdef SS_INC_COMDEF
	friend	MYTYPE	operator+(const _bstr_t& bstr,	const MYTYPE& str);
	friend	MYTYPE	operator+(const MYTYPE& str,	const _bstr_t& bstr);
#endif

	// -------------------------------------------------------------------------
	// Case changing functions
	// -------------------------------------------------------------------------
	// -------------------------------------------------------------------------
	MYTYPE& ToUpper()
	{
		//  Strictly speaking, this would be about the most portable way

		//	std::transform(begin(),
		//				   end(),
		//				   begin(),
		//				   std::bind2nd(SSToUpper<CT>(), std::locale()));

		// But practically speaking, this works faster

		if ( !empty() )
			ssupr(GetBuf(), size());

		return *this;
	}



	MYTYPE& ToLower()
	{
		//  Strictly speaking, this would be about the most portable way

		//	std::transform(begin(),
		//				   end(),
		//				   begin(),
		//				   std::bind2nd(SSToLower<CT>(), std::locale()));

		// But practically speaking, this works faster

		if ( !empty() )
			sslwr(GetBuf(), size());

		return *this;
	}



	MYTYPE& Normalize()
	{
		return Trim().ToLower();
	}


	// -------------------------------------------------------------------------
	// CStdStr -- Direct access to character buffer.  In the MS' implementation,
	// the at() function that we use here also calls _Freeze() providing us some
	// protection from multithreading problems associated with ref-counting.
	// -------------------------------------------------------------------------
	CT* GetBuf(int nMinLen=-1)
	{
		if ( static_cast<int>(size()) < nMinLen )
			resize(static_cast<MYSIZE>(nMinLen));

		return empty() ? const_cast<CT*>(data()) : &(at(0));
	}

	CT* SetBuf(int nLen)
	{
		nLen = ( nLen > 0 ? nLen : 0 );
		if ( capacity() < 1 && nLen == 0 )
			resize(1);

		resize(static_cast<MYSIZE>(nLen));
		return const_cast<CT*>(data());
	}
	void RelBuf(int nNewLen=-1)
	{
		resize(static_cast<MYSIZE>(nNewLen > -1 ? nNewLen : sslen(c_str())));
	}

	void BufferRel()		 { RelBuf(); }			// backwards compatability
	CT*  Buffer()			 { return GetBuf(); }	// backwards compatability
	CT*  BufferSet(int nLen) { return SetBuf(nLen);}// backwards compatability

	bool Equals(const CT* pT, bool bUseCase=false) const
	{	// get copy, THEN compare (thread safe)
		return  bUseCase ? compare(pT) == 0 : ssicmp(MYTYPE(*this), pT) == 0;
	} 

	// -------------------------------------------------------------------------
	// FUNCTION:  CStdStr::Load
	// REMARKS:
	//		Loads string from resource specified by nID
	//
	// PARAMETERS:
	//		nID - resource Identifier.  Purely a Win32 thing in this case
	//
	// RETURN VALUE:
	//		true if successful, false otherwise
	// -------------------------------------------------------------------------
#ifndef SS_ANSI
	bool Load(UINT nId, HMODULE hModule=NULL)
	{
		bool bLoaded		= false;	// set to true of we succeed.

	#ifdef _MFC_VER		// When in Rome...

		CString strRes;
		bLoaded				= FALSE != strRes.LoadString(nId);
		if ( bLoaded )
			*this			= strRes;

	#else
		
		// Get the resource name and module handle

		if ( NULL == hModule )
			hModule			= GetResourceHandle();

		PCTSTR szName		= MAKEINTRESOURCE((nId>>4)+1); // lifted 
		DWORD dwSize		= 0;

		// No sense continuing if we can't find the resource

		HRSRC hrsrc			= ::FindResource(hModule, szName, RT_STRING);

		if ( NULL == hrsrc )
			TRACE(_T("Cannot find resource %d: 0x%X"), nId, ::GetLastError());
		else if ( 0 == (dwSize = ::SizeofResource(hModule, hrsrc) / sizeof(CT)))
			TRACE(_T("Cant get size of resource %d 0x%X\n"),nId,GetLastError());
		else
		{
			bLoaded			= 0 != ssload(hModule, nId, GetBuf(dwSize), dwSize);
			ReleaseBuffer();
		}

	#endif

		if ( !bLoaded )
			TRACE(_T("String not loaded 0x%X\n"), ::GetLastError());

		return bLoaded;
	}
#endif
	
	// -------------------------------------------------------------------------
	// FUNCTION:  CStdStr::Format
	//		void _cdecl Formst(CStdStringA& PCSTR szFormat, ...)
	//		void _cdecl Format(PCSTR szFormat);
	//           
	// DESCRIPTION:
	//		This function does sprintf/wsprintf style formatting on CStdStringA
	//		objects.  It looks a lot like MFC's CString::Format.  Some people
	//		might even call this identical.  Fortunately, these people are now
	//		dead.
	//
	// PARAMETERS: 
	//		nId - ID of string resource holding the format string
	//		szFormat - a PCSTR holding the format specifiers
	//		argList - a va_list holding the arguments for the format specifiers.
	//
	// RETURN VALUE:  None.
	// -------------------------------------------------------------------------
	// formatting (using wsprintf style formatting)
	#ifndef SS_ANSI
	void Format(UINT nId, ...)
	{
		va_list argList;
		va_start(argList, nId);
		va_start(argList, nId);

		MYTYPE strFmt;
		if ( strFmt.Load(nId) )
			FormatV(strFmt, argList);

		va_end(argList);
	}
	#endif
	void Format(const CT* szFmt, ...)
	{
		va_list argList;
		va_start(argList, szFmt);
		FormatV(szFmt, argList);
		va_end(argList);
	}
	void AppendFormat(const CT* szFmt, ...)
	{
		va_list argList;
		va_start(argList, szFmt);
		AppendFormatV(szFmt, argList);
		va_end(argList);
	}

	#define MAX_FMT_TRIES		5	 // #of times we try 
	#define FMT_BLOCK_SIZE		2048 // # of bytes to increment per try
	#define BUFSIZE_1ST	256
	#define BUFSIZE_2ND 512
	#define STD_BUF_SIZE		1024

	// an efficient way to add formatted characters to the string.  You may only
	// add up to STD_BUF_SIZE characters at a time, though
	void AppendFormatV(const CT* szFmt, va_list argList)
	{
		CT szBuf[STD_BUF_SIZE];
	#ifdef SS_ANSI
		int nLen = ssvsprintf(szBuf, STD_BUF_SIZE-1, szFmt, argList);
	#else
		int nLen = ssnprintf(szBuf, STD_BUF_SIZE-1, szFmt, argList);
	#endif
		if ( 0 < nLen )
			append(szBuf, nLen);
	}

	// -------------------------------------------------------------------------
	// FUNCTION:  FormatV
	//		void FormatV(PCSTR szFormat, va_list, argList);
	//           
	// DESCRIPTION:
	//		This function formats the string with sprintf style format-specs. 
	//		It makes a general guess at required buffer size and then tries
	//		successively larger buffers until it finds one big enough or a
	//		threshold (MAX_FMT_TRIES) is exceeded.
	//
	// PARAMETERS: 
	//		szFormat - a PCSTR holding the format of the output
	//		argList - a Microsoft specific va_list for variable argument lists
	//
	// RETURN VALUE: 
	// -------------------------------------------------------------------------

	void FormatV(const CT* szFormat, va_list argList)
	{
	#ifdef SS_ANSI

		int nLen	= sslen(szFormat) + STD_BUF_SIZE;
		ssvsprintf(GetBuffer(nLen), nLen-1, szFormat, argList);
		ReleaseBuffer();

	#else

		CT* pBuf			= NULL;
		int nChars			= 1;
		int nUsed			= 0;
		size_type nActual	= 0;
		int nTry			= 0;

		do	
		{
			// Grow more than linearly (e.g. 512, 1536, 3072, etc)

			nChars			+= ((nTry+1) * FMT_BLOCK_SIZE);
			pBuf			= reinterpret_cast<CT*>(_alloca(sizeof(CT)*nChars));
			nUsed			= ssnprintf(pBuf, nChars-1, szFormat, argList);

			// Ensure proper NULL termination.

			nActual			= nUsed == -1 ? nChars-1 : SSMIN(nUsed, nChars-1);
			pBuf[nActual+1]= '\0';


		} while ( nUsed < 0 && nTry++ < MAX_FMT_TRIES );

		// assign whatever we managed to format

		assign(pBuf, nActual);

	#endif
	}
	

	// -------------------------------------------------------------------------
	// CString Facade Functions:
	//
	// The following methods are intended to allow you to use this class as a
	// drop-in replacement for CString.
	// -------------------------------------------------------------------------
	#ifndef SS_ANSI
		BSTR AllocSysString() const
		{
			ostring os;
			ssasn(os, *this);
			return ::SysAllocString(os.c_str());
		}
	#endif

	int Collate(PCMYSTR szThat) const
	{
		return sscoll(c_str(), length(), szThat, sslen(szThat));
	}

	int CollateNoCase(PCMYSTR szThat) const
	{
		return ssicoll(c_str(), length(), szThat, sslen(szThat));
	}

	int Compare(PCMYSTR szThat) const
	{
		return MYBASE::compare(szThat);	
	}

	int CompareNoCase(PCMYSTR szThat)	const
	{
		return ssicmp(c_str(), szThat);
	}

	int Delete(int nIdx, int nCount=1)
	{
		if ( nIdx < GetLength() )
			erase(static_cast<MYSIZE>(nIdx), static_cast<MYSIZE>(nCount));

		return GetLength();
	}

	void Empty()
	{
		erase();
	}

	int Find(CT ch) const
	{
		MYSIZE nIdx	= find_first_of(ch);
		return static_cast<int>(MYBASE::npos == nIdx  ? -1 : nIdx);
	}

	int Find(PCMYSTR szSub) const
	{
		MYSIZE nIdx	= find(szSub);
		return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
	}

	int Find(CT ch, int nStart) const
	{
		// CString::Find docs say add 1 to nStart when it's not zero
		// CString::Find code doesn't do that however.  We'll stick
		// with what the code does

		MYSIZE nIdx	= find_first_of(ch, static_cast<MYSIZE>(nStart));
		return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
	}

	int Find(PCMYSTR szSub, int nStart) const
	{
		// CString::Find docs say add 1 to nStart when it's not zero
		// CString::Find code doesn't do that however.  We'll stick
		// with what the code does

		MYSIZE nIdx	= find(szSub, static_cast<MYSIZE>(nStart));
		return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
	}

	int FindOneOf(PCMYSTR szCharSet) const
	{
		MYSIZE nIdx = find_first_of(szCharSet);
		return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
	}

#ifndef SS_ANSI
	void FormatMessage(PCMYSTR szFormat, ...) throw(std::exception)
	{
		va_list argList;
		va_start(argList, szFormat);
		PMYSTR szTemp;
		if ( ssfmtmsg(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
					   szFormat, 0, 0,
					   reinterpret_cast<PMYSTR>(&szTemp), 0, &argList) == 0 ||
			 szTemp == 0 )
		{
			throw std::runtime_error("out of memory");
		}
		*this = szTemp;
		LocalFree(szTemp);
		va_end(argList);
	}

	void FormatMessage(UINT nFormatId, ...) throw(std::exception)
	{
		MYTYPE sFormat;
		VERIFY(sFormat.LoadString(nFormatId) != 0);
		va_list argList;
		va_start(argList, nFormatId);
		PMYSTR szTemp;
		if ( ssfmtmsg(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
					   sFormat, 0, 0,
					   reinterpret_cast<PMYSTR>(&szTemp), 0, &argList) == 0 ||
			szTemp == 0)
		{
			throw std::runtime_error("out of memory");
		}
		*this = szTemp;
		LocalFree(szTemp);
		va_end(argList);
	}
#endif


	// -------------------------------------------------------------------------
	// GetXXXX -- Direct access to character buffer
	// -------------------------------------------------------------------------
	CT GetAt(int nIdx) const
	{
		return at(static_cast<MYSIZE>(nIdx));
	}

	CT* GetBuffer(int nMinLen=-1)
	{
		return GetBuf(nMinLen);
	}

	CT* GetBufferSetLength(int nLen)
	{
		return BufferSet(nLen);
	}

	// GetLength() -- MFC docs say this is the # of BYTES but
	// in truth it is the number of CHARACTERs (chars or wchar_ts)
	int GetLength() const
	{
		return static_cast<int>(length());
	}

	
	int Insert(int nIdx, CT ch)
	{
		if ( static_cast<MYSIZE>(nIdx) > size() -1 )
			append(1, ch);
		else
			insert(static_cast<MYSIZE>(nIdx), 1, ch);

		return GetLength();
	}
	int Insert(int nIdx, PCMYSTR sz)
	{
		if ( nIdx >= size() )
			append(sz, sslen(sz));
		else
			insert(static_cast<MYSIZE>(nIdx), sz);

		return GetLength();
	}

	bool IsEmpty() const
	{
		return empty();
	}

	MYTYPE Left(int nCount) const
	{
		return substr(0, static_cast<MYSIZE>(nCount)); 
	}

	#ifndef SS_ANSI
	bool LoadString(UINT nId)
	{
		return this->Load(nId);
	}
	#endif

	void MakeLower()
	{
		ToLower();
	}

	void MakeReverse()
	{
		std::reverse(begin(), end());
	}

	void MakeUpper()
	{ 
		ToUpper();
	}

	MYTYPE Mid(int nFirst ) const
	{
		return substr(static_cast<MYSIZE>(nFirst));
	}

	MYTYPE Mid(int nFirst, int nCount) const
	{
		return substr(static_cast<MYSIZE>(nFirst), static_cast<MYSIZE>(nCount));
	}

	void ReleaseBuffer(int nNewLen=-1)
	{
		RelBuf(nNewLen);
	}

	int Remove(CT ch)
	{
		MYSIZE nIdx		= 0;
		int nRemoved	= 0;
		while ( (nIdx=find_first_of(ch)) != MYBASE::npos )
		{
			erase(nIdx, 1);
			nRemoved++;
		}
		return nRemoved;
	}

	int Replace(CT chOld, CT chNew)
	{
		int nReplaced	= 0;
		for ( MYITER iter=begin(); iter != end(); iter++ )
		{
			if ( *iter == chOld )
			{
				*iter = chNew;
				nReplaced++;
			}
		}
		return nReplaced;
	}

	int Replace(PCMYSTR szOld, PCMYSTR szNew)
	{
		int nReplaced		= 0;
		MYSIZE nIdx			= 0;
		MYSIZE nOldLen		= sslen(szOld);
		if ( 0 == nOldLen )
			return 0;

		static const CT ch	= CT(0);
		MYSIZE nNewLen		= sslen(szNew);
		PCMYSTR szRealNew	= szNew == 0 ? &ch : szNew;

		while ( (nIdx=find(szOld, nIdx)) != MYBASE::npos )
		{
			replace(begin()+nIdx, begin()+nIdx+nOldLen, szRealNew);
			nReplaced++;
			nIdx += nNewLen;
		}
		return nReplaced;
	}

	int ReverseFind(CT ch) const
	{
		MYSIZE nIdx	= find_last_of(ch);
		return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
	}

	// ReverseFind overload that's not in CString but might be useful
	int ReverseFind(PCMYSTR szFind, MYSIZE pos=MYBASE::npos) const
	{
		MYSIZE nIdx	= rfind(0 == szFind ? MYTYPE() : szFind, pos);
		return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
	}

	MYTYPE Right(int nCount) const
	{
		nCount = SSMIN(nCount, static_cast<int>(size()));
		return substr(size()-static_cast<MYSIZE>(nCount));
	}

	void SetAt(int nIndex, CT ch)
	{
		ASSERT(size() > static_cast<MYSIZE>(nIndex));
		at(static_cast<MYSIZE>(nIndex))		= ch;
	}

	#ifndef SS_ANSI
		BSTR SetSysString(BSTR* pbstr) const
		{
			ostring os;
			ssasn(os, *this);
			if ( !::SysReAllocStringLen(pbstr, os.c_str(), os.length()) )
				throw std::runtime_error("out of memory");

			ASSERT(*pbstr != 0);
			return *pbstr;
		}
	#endif

	MYTYPE SpanExcluding(PCMYSTR szCharSet) const
	{
		return Left(find_first_of(szCharSet));
	}

	MYTYPE SpanIncluding(PCMYSTR szCharSet) const
	{
		return Left(find_first_not_of(szCharSet));
	}

	#if !defined(UNICODE) && !defined(SS_ANSI)

		// CString's OemToAnsi and AnsiToOem functions are available only in
		// Unicode builds.  However since we're a template we also need a
		// runtime check of CT and a reinterpret_cast to account for the fact
		// that CStdStringW gets instantiated even in non-Unicode builds.

		void AnsiToOem()
		{
			if ( sizeof(CT) == sizeof(char) && !empty() )
			{
				::CharToOem(reinterpret_cast<PCSTR>(c_str()),
							reinterpret_cast<PSTR>(GetBuf()));
			}
			else
			{
				ASSERT(false);
			}
		}

		void OemToAnsi()
		{
			if ( sizeof(CT) == sizeof(char) && !empty() )
			{
				::OemToChar(reinterpret_cast<PCSTR>(c_str()),
							reinterpret_cast<PSTR>(GetBuf()));
			}
			else
			{
				ASSERT(false);
			}
		}

	#endif
	

	// -------------------------------------------------------------------------
	// Trim and its variants
	// -------------------------------------------------------------------------
	MYTYPE& Trim()
	{
		return TrimLeft().TrimRight();
	}

	MYTYPE& TrimLeft()
	{
		erase(begin(), std::find_if(begin(),end(),NotSpace<CT>(std::locale())));
		return *this;
	}

	MYTYPE&  TrimLeft(CT tTrim)
	{
		erase(0, find_first_not_of(tTrim));
		return *this;
	}

	MYTYPE&  TrimLeft(PCMYSTR szTrimChars)
	{
		erase(0, find_first_not_of(szTrimChars));
		return *this;
	}

	MYTYPE& TrimRight()
	{
		std::locale loc;
		MYRITER it = std::find_if(rbegin(), rend(), NotSpace<CT>(loc));
		if ( rend() != it )
			erase(rend() - it);

		erase(it != rend() ? find_last_of(*it) + 1 : 0);
		return *this;
	}

	MYTYPE&  TrimRight(CT tTrim)
	{
		MYSIZE nIdx	= find_last_not_of(tTrim);
		erase(MYBASE::npos == nIdx ? 0 : ++nIdx);
		return *this;
	}

	MYTYPE&  TrimRight(PCMYSTR szTrimChars)
	{
		MYSIZE nIdx	= find_last_not_of(szTrimChars);
		erase(MYBASE::npos == nIdx ? 0 : ++nIdx);
		return *this;
	}

	void			FreeExtra()
	{
		MYTYPE mt;
		swap(mt);
		if ( !mt.empty() )
			assign(mt.c_str(), mt.size());
	}

	// I have intentionally not implemented the following CString
	// functions.   You cannot make them work without taking advantage
	// of implementation specific behavior.  However if you absolutely
	// MUST have them, uncomment out these lines for "sort-of-like"
	// their behavior.  You're on your own.

//	CT*				LockBuffer()	{ return GetBuf(); }// won't really lock
//	void			UnlockBuffer(); { }	// why have UnlockBuffer w/o LockBuffer?

	// Array-indexing operators.  Required because we defined an implicit cast
	// to operator const CT* (Thanks to Julian Selman for pointing this out)
	CT& operator[](int nIdx)
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	const CT& operator[](int nIdx) const
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	CT& operator[](unsigned int nIdx)
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	const CT& operator[](unsigned int nIdx) const
	{
		return MYBASE::operator[](static_cast<MYSIZE>(nIdx));
	}

	operator const CT*() const
	{
		return c_str();
	}

	// IStream related functions.  Useful in IPersistStream implementations

#ifdef SS_INC_COMDEF

	// struct SSSHDR - useful for non Std C++ persistence schemes.
	typedef struct SSSHDR
	{
		BYTE	byCtrl;
		ULONG	nChars;
	} SSSHDR;	// as in "Standard String Stream Header"

	#define SSSO_UNICODE	0x01	// the string is a wide string
	#define SSSO_COMPRESS	0x02	// the string is compressed

	// -------------------------------------------------------------------------
	// FUNCTION: StreamSize
	// REMARKS:
	//		Returns how many bytes it will take to StreamSave() this CStdString
	//		object to an IStream.
	// -------------------------------------------------------------------------
	ULONG StreamSize() const
	{
		// Control header plus string
		ASSERT(size()*sizeof(CT) < 0xffffffffUL - sizeof(SSSHDR));
		return (size() * sizeof(CT)) + sizeof(SSSHDR);
	}

	// -------------------------------------------------------------------------
	// FUNCTION: StreamSave
	// REMARKS:
	//		Saves this CStdString object to a COM IStream.
	// -------------------------------------------------------------------------
	HRESULT StreamSave(IStream* pStream) const
	{
		ASSERT(size()*sizeof(CT) < 0xffffffffUL - sizeof(SSSHDR));
		HRESULT hr		= E_FAIL;
		ASSERT(pStream != 0);
		SSSHDR hdr;
		hdr.byCtrl		= sizeof(CT) == 2 ? SSSO_UNICODE : 0;
		hdr.nChars		= size();


		if ( FAILED(hr=pStream->Write(&hdr, sizeof(SSSHDR), 0)) )
			TRACE(_T("StreamSave: Cannot write control header, ERR=0x%X\n"),hr);
		else if ( empty() )
			;		// nothing to write
		else if ( FAILED(hr=pStream->Write(c_str(), size()*sizeof(CT), 0)) )
			TRACE(_T("StreamSave: Cannot write string to stream 0x%X\n"), hr);

		return hr;
	}


	// -------------------------------------------------------------------------
	// FUNCTION: StreamLoad
	// REMARKS:
	//		This method loads the object from an IStream.
	// -------------------------------------------------------------------------
	HRESULT StreamLoad(IStream* pStream)
	{
		ASSERT(pStream != 0);
		SSSHDR hdr;
		HRESULT hr			= E_FAIL;

		if ( FAILED(hr=pStream->Read(&hdr, sizeof(SSSHDR), 0)) )
		{
			TRACE(_T("StreamLoad: Cant read control header, ERR=0x%X\n"), hr);
		}
		else if ( hdr.nChars > 0 )
		{
			ULONG nRead		= 0;
			PMYSTR pMyBuf	= BufferSet(hdr.nChars);

			// If our character size matches the character size of the string
			// we're trying to read, then we can read it directly into our
			// buffer. Otherwise, we have to read into an intermediate buffer
			// and convert.
			
			if ( (hdr.byCtrl & SSSO_UNICODE) != 0 )
			{
				ULONG nBytes	= hdr.nChars * sizeof(wchar_t);
				if ( sizeof(CT) == sizeof(wchar_t) )
				{
					if ( FAILED(hr=pStream->Read(pMyBuf, nBytes, &nRead)) )
						TRACE(_T("StreamLoad: Cannot read string: 0x%X\n"), hr);
				}
				else
				{	
					PWSTR pBufW = reinterpret_cast<PWSTR>(_alloca((nBytes)+1));
					if ( FAILED(hr=pStream->Read(pBufW, nBytes, &nRead)) )
						TRACE(_T("StreamLoad: Cannot read string: 0x%X\n"), hr);
					else
						sscpy(pMyBuf, pBufW, hdr.nChars);
				}
			}
			else
			{
				ULONG nBytes	= hdr.nChars * sizeof(char);
				if ( sizeof(CT) == sizeof(char) )
				{
					if ( FAILED(hr=pStream->Read(pMyBuf, nBytes, &nRead)) )
						TRACE(_T("StreamLoad: Cannot read string: 0x%X\n"), hr);
				}
				else
				{
					PSTR pBufA = reinterpret_cast<PSTR>(_alloca(nBytes));
					if ( FAILED(hr=pStream->Read(pBufA, hdr.nChars, &nRead)) )
						TRACE(_T("StreamLoad: Cannot read string: 0x%X\n"), hr);
					else
						sscpy(pMyBuf, pBufA, hdr.nChars);
				}
			}
		}
		else
		{
			this->erase();
		}
		return hr;
	}
#endif // #ifdef SS_INC_COMDEF

#ifndef SS_ANSI

	// SetResourceHandle/GetResourceHandle.  In MFC builds, these map directly
	// to AfxSetResourceHandle and AfxGetResourceHandle.  In non-MFC builds they
	// point to a single static HINST so that those who call the member
	// functions that take resource IDs can provide an alternate HINST of a DLL
	// to search.  This is not exactly the list of HMODULES that MFC provides
	// but it's better than nothing.

	#ifdef _MFC_VER
		static void SetResourceHandle(HMODULE hNew)
		{
			AfxSetResourceHandle(hNew);
		}
		static HMODULE GetResourceHandle()
		{
			return AfxGetResourceHandle();
		}
	#else
		static void SetResourceHandle(HMODULE hNew)
		{
			SSResourceHandle() = hNew;
		}
		static HMODULE GetResourceHandle()
		{
			return SSResourceHandle();
		}
	#endif

#endif
};



// -----------------------------------------------------------------------------
// CStdStr friend addition functions defined as inline
// -----------------------------------------------------------------------------
template<typename CT>
inline
CStdStr<CT> operator+(const  CStdStr<CT>& str1, const  CStdStr<CT>& str2)
{
	CStdStr<CT> strRet(SSREF(str1));
	strRet.append(str2);
	return strRet;
}

template<typename CT>	
inline
CStdStr<CT> operator+(const  CStdStr<CT>& str, CT t)
{
	// this particular overload is needed for disabling reference counting
	// though it's only an issue from line 1 to line 2

	CStdStr<CT> strRet(SSREF(str));	// 1
	strRet.append(1, t);				// 2
	return strRet;
}

template<typename CT>
inline
CStdStr<CT> operator+(const  CStdStr<CT>& str, PCSTR pA)
{
	return CStdStr<CT>(str) + CStdStr<CT>(pA);
}

template<typename CT>
inline
CStdStr<CT> operator+(PCSTR pA, const  CStdStr<CT>& str)
{
	CStdStr<CT> strRet(pA);
	strRet.append(str);
	return strRet;
}

template<typename CT>
inline
CStdStr<CT> operator+(const CStdStr<CT>& str, PCWSTR pW)
{ 
	return CStdStr<CT>(SSREF(str)) + CStdStr<CT>(pW);
}

template<typename CT>
inline
CStdStr<CT> operator+(PCWSTR pW, const CStdStr<CT>& str)
{
	CStdStr<CT> strRet(pW);
	strRet.append(str);
	return strRet;
}

#ifdef SS_INC_COMDEF
	template<typename CT>
	inline
	CStdStr<CT> operator+(const _bstr_t& bstr, const CStdStr<CT>& str)
	{
		return static_cast<const CT*>(bstr) + str;
	}

	template<typename CT>
	inline
	CStdStr<CT> operator+(const CStdStr<CT>& str, const _bstr_t& bstr)
	{
		return str + static_cast<const CT*>(bstr);
	}
#endif




// -----------------------------------------------------------------------------
// These versions of operator+ provided by Scott Hathaway in order to allow
// CStdString to build on Sun Unix systems.
// -----------------------------------------------------------------------------

#if defined(__SUNPRO_CC_COMPAT) || defined(__SUNPRO_CC)

// Made non-template versions due to "undefined" errors on Sun Forte compiler
// when linking with friend template functions
inline
CStdStr<wchar_t> operator+(const CStdStr<wchar_t>& str1,
						   const CStdStr<wchar_t>& str2)
{
	CStdStr<wchar_t> strRet(SSREF(str1));
	strRet.append(str2);
	return strRet;
}

inline
CStdStr<wchar_t> operator+(const CStdStr<wchar_t>& str, wchar_t t)
{
	// this particular overload is needed for disabling reference counting
	// though it's only an issue from line 1 to line 2

	CStdStr<wchar_t> strRet(SSREF(str));	// 1
	strRet.append(1, t);					// 2
	return strRet;
}

inline
CStdStr<wchar_t> operator+(const CStdStr<wchar_t>& str, PCWSTR pW)
{
	return CStdStr<wchar_t>(str) + CStdStr<wchar_t>(pW);
}

inline
CStdStr<wchar_t> operator+(PCWSTR pA, const  CStdStr<wchar_t>& str)
{
	CStdStr<wchar_t> strRet(pA);
	strRet.append(str);
	return strRet;
}

inline
CStdStr<wchar_t> operator+(const CStdStr<wchar_t>& str, PCSTR pW)
{ 
	return CStdStr<wchar_t>(SSREF(str)) + CStdStr<wchar_t>(pW);
}

inline
CStdStr<wchar_t> operator+(PCSTR pW, const CStdStr<wchar_t>& str)
{
	CStdStr<wchar_t> strRet(pW);
	strRet.append(str);
	return strRet;
}

inline
CStdStr<char> operator+(const  CStdStr<char>& str1, const  CStdStr<char>& str2)
{
	CStdStr<char> strRet(SSREF(str1));
	strRet.append(str2);
	return strRet;
}

inline
CStdStr<char> operator+(const  CStdStr<char>& str, char t)
{
	// this particular overload is needed for disabling reference counting
	// though it's only an issue from line 1 to line 2

	CStdStr<char> strRet(SSREF(str));	// 1
	strRet.append(1, t);				// 2
	return strRet;
}

inline
CStdStr<char> operator+(const  CStdStr<char>& str, PCSTR pA)
{
	return CStdStr<char>(str) + CStdStr<char>(pA);
}

inline
CStdStr<char> operator+(PCSTR pA, const  CStdStr<char>& str)
{
	CStdStr<char> strRet(pA);
	strRet.append(str);
	return strRet;
}

inline
CStdStr<char> operator+(const CStdStr<char>& str, PCWSTR pW)
{ 
	return CStdStr<char>(SSREF(str)) + CStdStr<char>(pW);
}

inline
CStdStr<char> operator+(PCWSTR pW, const CStdStr<char>& str)
{
	CStdStr<char> strRet(pW);
	strRet.append(str);
	return strRet;
}


#endif // defined(__SUNPRO_CC_COMPAT) || defined(__SUNPRO_CC)


// =============================================================================
//						END OF CStdStr INLINE FUNCTION DEFINITIONS
// =============================================================================

//	Now typedef our class names based upon this humongous template

typedef CStdStr<char>		CStdStringA;	// a better std::string
typedef CStdStr<wchar_t>	CStdStringW;	// a better std::wstring
typedef CStdStr<OLECHAR>	CStdStringO;	// almost always CStdStringW

#ifndef SS_ANSI
	// SSResourceHandle: our MFC-like resource handle
	inline HMODULE& SSResourceHandle()
	{
		static HMODULE hModuleSS	= GetModuleHandle(0);
		return hModuleSS;
	}
#endif


// In MFC builds, define some global serialization operators
// Special operators that allow us to serialize CStdStrings to CArchives.
// Note that we use an intermediate CString object in order to ensure that
// we use the exact same format.

#ifdef _MFC_VER
	inline CArchive& AFXAPI operator<<(CArchive& ar, const CStdStringA& strA)
	{
		CString strTemp	= strA;
		return ar << strTemp;
	}
	inline CArchive& AFXAPI operator<<(CArchive& ar, const CStdStringW& strW)
	{
		CString strTemp	= strW;
		return ar << strTemp;
	}

	inline CArchive& AFXAPI operator>>(CArchive& ar, CStdStringA& strA)
	{
		CString strTemp;
		ar >> strTemp;
		strA = strTemp;
		return ar;
	}
	inline CArchive& AFXAPI operator>>(CArchive& ar, CStdStringW& strW)
	{
		CString strTemp;
		ar >> strTemp;
		strW = strTemp;
		return ar;
	}
#endif	// #ifdef _MFC_VER -- (i.e. is this MFC?)



// -----------------------------------------------------------------------------
// HOW TO EXPORT CSTDSTRING FROM A DLL
//
// If you want to export CStdStringA and CStdStringW from a DLL, then all you
// need to
//		1.	make sure that all components link to the same DLL version
//			of the CRT (not the static one).
//		2.	Uncomment the 3 lines of code below
//		3.	#define 2 macros per the instructions in MS KnowledgeBase
//			article Q168958.  The macros are:
//
//		MACRO		DEFINTION WHEN EXPORTING		DEFINITION WHEN IMPORTING
//		-----		------------------------		-------------------------
//		SSDLLEXP	(nothing, just #define it)		extern
//		SSDLLSPEC	__declspec(dllexport)			__declspec(dllimport)
//
//		Note that these macros must be available to ALL clients who want to 
//		link to the DLL and use the class.  If they 
// -----------------------------------------------------------------------------
//#pragma warning(disable:4231) // non-standard extension ("extern template")
//	SSDLLEXP template class SSDLLSPEC CStdStr<char>;
//	SSDLLEXP template class SSDLLSPEC CStdStr<wchar_t>;


// -----------------------------------------------------------------------------
// GLOBAL FUNCTION:  WUFormat
//		CStdStringA WUFormat(UINT nId, ...);
//		CStdStringA WUFormat(PCSTR szFormat, ...);
//
// REMARKS:
//		This function allows the caller for format and return a CStdStringA
//		object with a single line of code.
// -----------------------------------------------------------------------------
#ifdef SS_ANSI
#else
	inline CStdStringA WUFormatA(UINT nId, ...)
	{
		va_list argList;
		va_start(argList, nId);

		CStdStringA strFmt;
		CStdStringA strOut;
		if ( strFmt.Load(nId) )
			strOut.FormatV(strFmt, argList);

		va_end(argList);
		return strOut;
	}
	inline CStdStringA WUFormatA(PCSTR szFormat, ...)
	{
		va_list argList;
		va_start(argList, szFormat);
		CStdStringA strOut;
		strOut.FormatV(szFormat, argList);
		va_end(argList);
		return strOut;
	}

	inline CStdStringW WUFormatW(UINT nId, ...)
	{
		va_list argList;
		va_start(argList, nId);

		CStdStringW strFmt;
		CStdStringW strOut;
		if ( strFmt.Load(nId) )
			strOut.FormatV(strFmt, argList);

		va_end(argList);
		return strOut;
	}
	inline CStdStringW WUFormatW(PCWSTR szwFormat, ...)
	{
		va_list argList;
		va_start(argList, szwFormat);
		CStdStringW strOut;
		strOut.FormatV(szwFormat, argList);
		va_end(argList);
		return strOut;
	}
#endif // #ifdef SS_ANSI

#ifdef SS_ANSI
#else
	// -------------------------------------------------------------------------
	// FUNCTION: WUSysMessage
	//	 CStdStringA WUSysMessageA(DWORD dwError, DWORD dwLangId=SS_DEFLANGID);
	//	 CStdStringW WUSysMessageW(DWORD dwError, DWORD dwLangId=SS_DEFLANGID);
	//           
	// DESCRIPTION:
	//	 This function simplifies the process of obtaining a string equivalent
	//	 of a system error code returned from GetLastError().  You simply
	//	 supply the value returned by GetLastError() to this function and the
	//	 corresponding system string is returned in the form of a CStdStringA.
	//
	// PARAMETERS: 
	//	 dwError - a DWORD value representing the error code to be translated
	//	 dwLangId - the language id to use.  defaults to english.
	//
	// RETURN VALUE: 
	//	 a CStdStringA equivalent of the error code.  Currently, this function
	//	 only returns either English of the system default language strings.  
	// -------------------------------------------------------------------------
	#define SS_DEFLANGID MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT)
	inline CStdStringA WUSysMessageA(DWORD dwError, DWORD dwLangId=SS_DEFLANGID)
	{
		CHAR szBuf[512];

		if ( 0 != ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError,
								   dwLangId, szBuf, 511, NULL) )
			return WUFormatA("%s (0x%X)", szBuf, dwError);
		else
 			return WUFormatA("Unknown error (0x%X)", dwError);
	}
	inline CStdStringW WUSysMessageW(DWORD dwError, DWORD dwLangId=SS_DEFLANGID)
	{
		WCHAR szBuf[512];

		if ( 0 != ::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError,
								   dwLangId, szBuf, 511, NULL) )
			return WUFormatW(L"%s (0x%X)", szBuf, dwError);
		else
 			return WUFormatW(L"Unknown error (0x%X)", dwError);
	}
#endif

// Define TCHAR based friendly names for some of these functions

#ifdef UNICODE
	#define CStdString				CStdStringW
	#define WUSysMessage			WUSysMessageW
	#define WUFormat				WUFormatW
#else
	#define CStdString				CStdStringA
	#define WUSysMessage			WUSysMessageA
	#define WUFormat				WUFormatA
#endif

// ...and some shorter names for the space-efficient

#define WUSysMsg					WUSysMessage
#define WUSysMsgA					WUSysMessageA
#define WUSysMsgW					WUSysMessageW
#define WUFmtA						WUFormatA
#define	WUFmtW						WUFormatW
#define WUFmt						WUFormat
#define WULastErrMsg()				WUSysMessage(::GetLastError())
#define WULastErrMsgA()				WUSysMessageA(::GetLastError())
#define WULastErrMsgW()				WUSysMessageW(::GetLastError())


// -----------------------------------------------------------------------------
// FUNCTIONAL COMPARATORS:
// REMARKS:
//		These structs are derived from the std::binary_function template.  They
//		give us functional classes (which may be used in Standard C++ Library
//		collections and algorithms) that perform case-insensitive comparisons of
//		CStdString objects.  This is useful for maps in which the key may be the
//		 proper string but in the wrong case.
// -----------------------------------------------------------------------------
#define StdStringLessNoCaseW		SSLNCW	// avoid VC compiler warning 4786
#define StdStringEqualsNoCaseW		SSENCW		
#define StdStringLessNoCaseA		SSLNCA		
#define StdStringEqualsNoCaseA		SSENCA		

#ifdef UNICODE
	#define StdStringLessNoCase		SSLNCW		
	#define StdStringEqualsNoCase	SSENCW		
#else
	#define StdStringLessNoCase		SSLNCA		
	#define StdStringEqualsNoCase	SSENCA		
#endif

struct StdStringLessNoCaseW
	: std::binary_function<CStdStringW, CStdStringW, bool>
{
	inline
	bool operator()(const CStdStringW& sLeft, const CStdStringW& sRight) const
	{ return ssicmp(sLeft.c_str(), sRight.c_str()) < 0; }
};
struct StdStringEqualsNoCaseW
	: std::binary_function<CStdStringW, CStdStringW, bool>
{
	inline
	bool operator()(const CStdStringW& sLeft, const CStdStringW& sRight) const
	{ return ssicmp(sLeft.c_str(), sRight.c_str()) == 0; }
};
struct StdStringLessNoCaseA
	: std::binary_function<CStdStringA, CStdStringA, bool>
{
	inline
	bool operator()(const CStdStringA& sLeft, const CStdStringA& sRight) const
	{ return ssicmp(sLeft.c_str(), sRight.c_str()) < 0; }
};
struct StdStringEqualsNoCaseA
	: std::binary_function<CStdStringA, CStdStringA, bool>
{
	inline
	bool operator()(const CStdStringA& sLeft, const CStdStringA& sRight) const
	{ return ssicmp(sLeft.c_str(), sRight.c_str()) == 0; }
};

// If we had to define our own version of TRACE above, get rid of it now

#ifdef TRACE_DEFINED_HERE
	#undef TRACE
	#undef TRACE_DEFINED_HERE
#endif


#endif	// #ifndef STDSTRING_H
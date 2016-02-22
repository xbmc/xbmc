#pragma once
#include "../os.h"
#include <string>
#include <stdint.h>
#include <vector>

#if defined(_WIN32) && !defined(va_copy)
#define va_copy(dst, src) ((dst) = (src))
#endif

// =============================================================================
//  FILE:  StdString.h
//  AUTHOR:  Joe O'Leary (with outside help noted in comments)
//
//    If you find any bugs in this code, please let me know:
//
//        jmoleary@earthlink.net
//        http://www.joeo.net/stdstring.htm (a bit outdated)
//
//      The latest version of this code should always be available at the
//      following link:
//
//              http://www.joeo.net/code/StdString.zip (Dec 6, 2003)
//
//
//  REMARKS:
//    This header file declares the CStdStr template.  This template derives
//    the Standard C++ Library basic_string<> template and add to it the
//    the following conveniences:
//      - The full MFC CString set of functions (including implicit cast)
//      - writing to/reading from COM IStream interfaces
//      - Functional objects for use in STL algorithms
//
//    From this template, we intstantiate two classes:  CStdStringA and
//    CStdStringW.  The name "CStdString" is just a #define of one of these,
//    based upone the UNICODE macro setting
//
//    This header also declares our own version of the MFC/ATL UNICODE-MBCS
//    conversion macros.  Our version looks exactly like the Microsoft's to
//    facilitate portability.
//
//  NOTE:
//    If you you use this in an MFC or ATL build, you should include either
//    afx.h or atlbase.h first, as appropriate.
//
//  PEOPLE WHO HAVE CONTRIBUTED TO THIS CLASS:
//
//    Several people have helped me iron out problems and othewise improve
//    this class.  OK, this is a long list but in my own defense, this code
//    has undergone two major rewrites.  Many of the improvements became
//    necessary after I rewrote the code as a template.  Others helped me
//    improve the CString facade.
//
//    Anyway, these people are (in chronological order):
//
//      - Pete the Plumber (???)
//      - Julian Selman
//      - Chris (of Melbsys)
//      - Dave Plummer
//      - John C Sipos
//      - Chris Sells
//      - Nigel Nunn
//      - Fan Xia
//      - Matthew Williams
//      - Carl Engman
//      - Mark Zeren
//      - Craig Watson
//      - Rich Zuris
//      - Karim Ratib
//      - Chris Conti
//      - Baptiste Lepilleur
//      - Greg Pickles
//      - Jim Cline
//      - Jeff Kohn
//      - Todd Heckel
//      - Ullrich Poll�hne
//      - Joe Vitaterna
//      - Joe Woodbury
//      - Aaron (no last name)
//      - Joldakowski (???)
//      - Scott Hathaway
//      - Eric Nitzche
//      - Pablo Presedo
//      - Farrokh Nejadlotfi
//      - Jason Mills
//      - Igor Kholodov
//      - Mike Crusader
//      - John James
//      - Wang Haifeng
//      - Tim Dowty
//          - Arnt Witteveen
//          - Glen Maynard
//          - Paul DeMarco
//          - Bagira (full name?)
//          - Ronny Schulz
//          - Jakko Van Hunen
//      - Charles Godwin
//      - Henk Demper
//      - Greg Marr
//      - Bill Carducci
//      - Brian Groose
//      - MKingman
//      - Don Beusee
//
//  REVISION HISTORY
//
//    2005-JAN-10 - Thanks to Don Beusee for pointing out the danger in mapping
//          length-checked formatting functions to non-length-checked
//          CRT equivalents.  Also thanks to him for motivating me to
//          optimize my implementation of Replace()
//
//    2004-APR-22 - A big, big thank you to "MKingman" (whoever you are) for
//          finally spotting a silly little error in StdCodeCvt that
//          has been causing me (and users of CStdString) problems for
//          years in some relatively rare conversions.  I had reversed
//          two length arguments.
//
//    2003-NOV-24 - Thanks to a bunch of people for helping me clean up many
//          compiler warnings (and yes, even a couple of actual compiler
//          errors).  These include Henk Demper for figuring out how
//          to make the Intellisense work on with CStdString on VC6,
//          something I was never able to do.  Greg Marr pointed out
//          a compiler warning about an unreferenced symbol and a
//          problem with my version of Load in MFC builds.  Bill
//          Carducci took a lot of time with me to help me figure out
//          why some implementations of the Standard C++ Library were
//          returning error codes for apparently successful conversions
//          between ASCII and UNICODE.  Finally thanks to Brian Groose
//          for helping me fix compiler signed unsigned warnings in
//          several functions.
//
//    2003-JUL-10 - Thanks to Charles Godwin for making me realize my 'FmtArg'
//          fixes had inadvertently broken the DLL-export code (which is
//                  normally commented out.  I had to move it up higher.  Also
//          this helped me catch a bug in ssicoll that would prevent
//                  compilation, otherwise.
//
//    2003-MAR-14 - Thanks to Jakko Van Hunen for pointing out a copy-and-paste
//                  bug in one of the overloads of FmtArg.
//
//    2003-MAR-10 - Thanks to Ronny Schulz for (twice!) sending me some changes
//                  to help CStdString build on SGI and for pointing out an
//                  error in placement of my preprocessor macros for ssfmtmsg.
//
//    2002-NOV-26 - Thanks to Bagira for pointing out that my implementation of
//                  SpanExcluding was not properly handling the case in which
//                  the string did NOT contain any of the given characters
//
//    2002-OCT-21 - Many thanks to Paul DeMarco who was invaluable in helping me
//                  get this code working with Borland's free compiler as well
//                  as the Dev-C++ compiler (available free at SourceForge).
//
//    2002-SEP-13 - Thanks to Glen Maynard who helped me get rid of some loud
//                  but harmless warnings that were showing up on g++.  Glen
//                  also pointed out that some pre-declarations of FmtArg<>
//                  specializations were unnecessary (and no good on G++)
//
//    2002-JUN-26 - Thanks to Arnt Witteveen for pointing out that I was using
//                  static_cast<> in a place in which I should have been using
//                  reinterpret_cast<> (the ctor for unsigned char strings).
//                  That's what happens when I don't unit-test properly!
//                  Arnt also noticed that CString was silently correcting the
//                  'nCount' argument to Left() and Right() where CStdString was
//                  not (and crashing if it was bad).  That is also now fixed!
//
//    2002-FEB-25 - Thanks to Tim Dowty for pointing out (and giving me the fix
//          for) a conversion problem with non-ASCII MBCS characters.
//          CStdString is now used in my favorite commercial MP3 player!
//
//    2001-DEC-06 - Thanks to Wang Haifeng for spotting a problem in one of the
//          assignment operators (for _bstr_t) that would cause compiler
//          errors when refcounting protection was turned off.
//
//    2001-NOV-27 - Remove calls to operator!= which involve reverse_iterators
//          due to a conflict with the rel_ops operator!=.  Thanks to
//          John James for pointing this out.
//
//    2001-OCT-29 - Added a minor range checking fix for the Mid function to
//          make it as forgiving as CString's version is.  Thanks to
//          Igor Kholodov for noticing this.
//          - Added a specialization of std::swap for CStdString.  Thanks
//          to Mike Crusader for suggesting this!  It's commented out
//          because you're not supposed to inject your own code into the
//          'std' namespace.  But if you don't care about that, it's
//          there if you want it
//          - Thanks to Jason Mills for catching a case where CString was
//          more forgiving in the Delete() function than I was.
//
//    2001-JUN-06 - I was violating the Standard name lookup rules stated
//          in [14.6.2(3)].  None of the compilers I've tried so
//          far apparently caught this but HP-UX aCC 3.30 did.  The
//          fix was to add 'this->' prefixes in many places.
//          Thanks to Farrokh Nejadlotfi for this!
//
//    2001-APR-27 - StreamLoad was calculating the number of BYTES in one
//          case, not characters.  Thanks to Pablo Presedo for this.
//
//    2001-FEB-23 - Replace() had a bug which caused infinite loops if the
//          source string was empty.  Fixed thanks to Eric Nitzsche.
//
//    2001-FEB-23 - Scott Hathaway was a huge help in providing me with the
//          ability to build CStdString on Sun Unix systems.  He
//          sent me detailed build reports about what works and what
//          does not.  If CStdString compiles on your Unix box, you
//          can thank Scott for it.
//
//    2000-DEC-29 - Joldakowski noticed one overload of Insert failed to do a
//          range check as CString's does.  Now fixed -- thanks!
//
//    2000-NOV-07 - Aaron pointed out that I was calling static member
//          functions of char_traits via a temporary.  This was not
//          technically wrong, but it was unnecessary and caused
//          problems for poor old buggy VC5.  Thanks Aaron!
//
//    2000-JUL-11 - Joe Woodbury noted that the CString::Find docs don't match
//          what the CString::Find code really ends up doing.   I was
//          trying to match the docs.  Now I match the CString code
//          - Joe also caught me truncating strings for GetBuffer() calls
//          when the supplied length was less than the current length.
//
//    2000-MAY-25 - Better support for STLPORT's Standard library distribution
//          - Got rid of the NSP macro - it interfered with Koenig lookup
//          - Thanks to Joe Woodbury for catching a TrimLeft() bug that
//          I introduced in January.  Empty strings were not getting
//          trimmed
//
//    2000-APR-17 - Thanks to Joe Vitaterna for pointing out that ReverseFind
//          is supposed to be a const function.
//
//    2000-MAR-07 - Thanks to Ullrich Poll�hne for catching a range bug in one
//          of the overloads of assign.
//
//    2000-FEB-01 - You can now use CStdString on the Mac with CodeWarrior!
//          Thanks to Todd Heckel for helping out with this.
//
//    2000-JAN-23 - Thanks to Jim Cline for pointing out how I could make the
//          Trim() function more efficient.
//          - Thanks to Jeff Kohn for prompting me to find and fix a typo
//          in one of the addition operators that takes _bstr_t.
//          - Got rid of the .CPP file -  you only need StdString.h now!
//
//    1999-DEC-22 - Thanks to Greg Pickles for helping me identify a problem
//          with my implementation of CStdString::FormatV in which
//          resulting string might not be properly NULL terminated.
//
//    1999-DEC-06 - Chris Conti pointed yet another basic_string<> assignment
//          bug that MS has not fixed.  CStdString did nothing to fix
//          it either but it does now!  The bug was: create a string
//          longer than 31 characters, get a pointer to it (via c_str())
//          and then assign that pointer to the original string object.
//          The resulting string would be empty.  Not with CStdString!
//
//    1999-OCT-06 - BufferSet was erasing the string even when it was merely
//          supposed to shrink it.  Fixed.  Thanks to Chris Conti.
//          - Some of the Q172398 fixes were not checking for assignment-
//          to-self.  Fixed.  Thanks to Baptiste Lepilleur.
//
//    1999-AUG-20 - Improved Load() function to be more efficient by using
//          SizeOfResource().  Thanks to Rich Zuris for this.
//          - Corrected resource ID constructor, again thanks to Rich.
//          - Fixed a bug that occurred with UNICODE characters above
//          the first 255 ANSI ones.  Thanks to Craig Watson.
//          - Added missing overloads of TrimLeft() and TrimRight().
//          Thanks to Karim Ratib for pointing them out
//
//    1999-JUL-21 - Made all calls to GetBuf() with no args check length first.
//
//    1999-JUL-10 - Improved MFC/ATL independence of conversion macros
//          - Added SS_NO_REFCOUNT macro to allow you to disable any
//          reference-counting your basic_string<> impl. may do.
//          - Improved ReleaseBuffer() to be as forgiving as CString.
//          Thanks for Fan Xia for helping me find this and to
//          Matthew Williams for pointing it out directly.
//
//    1999-JUL-06 - Thanks to Nigel Nunn for catching a very sneaky bug in
//          ToLower/ToUpper.  They should call GetBuf() instead of
//          data() in order to ensure the changed string buffer is not
//          reference-counted (in those implementations that refcount).
//
//    1999-JUL-01 - Added a true CString facade.  Now you can use CStdString as
//          a drop-in replacement for CString.  If you find this useful,
//          you can thank Chris Sells for finally convincing me to give
//          in and implement it.
//          - Changed operators << and >> (for MFC CArchive) to serialize
//          EXACTLY as CString's do.  So now you can send a CString out
//          to a CArchive and later read it in as a CStdString.   I have
//          no idea why you would want to do this but you can.
//
//    1999-JUN-21 - Changed the CStdString class into the CStdStr template.
//          - Fixed FormatV() to correctly decrement the loop counter.
//          This was harmless bug but a bug nevertheless.  Thanks to
//          Chris (of Melbsys) for pointing it out
//          - Changed Format() to try a normal stack-based array before
//          using to _alloca().
//          - Updated the text conversion macros to properly use code
//          pages and to fit in better in MFC/ATL builds.  In other
//          words, I copied Microsoft's conversion stuff again.
//          - Added equivalents of CString::GetBuffer, GetBufferSetLength
//          - new sscpy() replacement of CStdString::CopyString()
//          - a Trim() function that combines TrimRight() and TrimLeft().
//
//    1999-MAR-13 - Corrected the "NotSpace" functional object to use _istpace()
//          instead of _isspace()   Thanks to Dave Plummer for this.
//
//    1999-FEB-26 - Removed errant line (left over from testing) that #defined
//          _MFC_VER.  Thanks to John C Sipos for noticing this.
//
//    1999-FEB-03 - Fixed a bug in a rarely-used overload of operator+() that
//          caused infinite recursion and stack overflow
//          - Added member functions to simplify the process of
//          persisting CStdStrings to/from DCOM IStream interfaces
//          - Added functional objects (e.g. StdStringLessNoCase) that
//          allow CStdStrings to be used as keys STL map objects with
//          case-insensitive comparison
//          - Added array indexing operators (i.e. operator[]).  I
//          originally assumed that these were unnecessary and would be
//          inherited from basic_string.  However, without them, Visual
//          C++ complains about ambiguous overloads when you try to use
//          them.  Thanks to Julian Selman to pointing this out.
//
//    1998-FEB-?? - Added overloads of assign() function to completely account
//          for Q172398 bug.  Thanks to "Pete the Plumber" for this
//
//    1998-FEB-?? - Initial submission
//
// COPYRIGHT:
//    2002 Joseph M. O'Leary.  This code is 100% free.  Use it anywhere you
//      want.  Rewrite it, restructure it, whatever.  If you can write software
//      that makes money off of it, good for you.  I kinda like capitalism.
//      Please don't blame me if it causes your $30 billion dollar satellite
//      explode in orbit.  If you redistribute it in any form, I'd appreciate it
//      if you would leave this notice here.
// =============================================================================

// Avoid multiple inclusion

#ifndef STDSTRING_H
#define STDSTRING_H

// When using VC, turn off browser references
// Turn off unavoidable compiler warnings

#if defined(_MSC_VER) && (_MSC_VER > 1100)
  #pragma component(browser, off, references, "CStdString")
  #pragma warning (disable : 4290) // C++ Exception Specification ignored
  #pragma warning (disable : 4127) // Conditional expression is constant
  #pragma warning (disable : 4097) // typedef name used as synonym for class name
#endif

// Borland warnings to turn off

#ifdef __BORLANDC__
    #pragma option push -w-inl
//  #pragma warn -inl   // Turn off inline function warnings
#endif

// SS_IS_INTRESOURCE
// -----------------
//    A copy of IS_INTRESOURCE from VC7.  Because old VC6 version of winuser.h
//    doesn't have this.

#define SS_IS_INTRESOURCE(_r) (false)

#if !defined (SS_ANSI) && defined(_MSC_VER)
  #undef SS_IS_INTRESOURCE
  #if defined(_WIN64)
    #define SS_IS_INTRESOURCE(_r) (((unsigned __int64)(_r) >> 16) == 0)
  #else
    #define SS_IS_INTRESOURCE(_r) (((unsigned long)(_r) >> 16) == 0)
  #endif
#endif


// MACRO: SS_UNSIGNED
// ------------------
//      This macro causes the addition of a constructor and assignment operator
//      which take unsigned characters.  CString has such functions and in order
//      to provide maximum CString-compatability, this code needs them as well.
//      In practice you will likely never need these functions...

//#define SS_UNSIGNED

#ifdef SS_ALLOW_UNSIGNED_CHARS
  #define SS_UNSIGNED
#endif

// MACRO: SS_SAFE_FORMAT
// ---------------------
//      This macro provides limited compatability with a questionable CString
//      "feature".  You can define it in order to avoid a common problem that
//      people encounter when switching from CString to CStdString.
//
//      To illustrate the problem -- With CString, you can do this:
//
//          CString sName("Joe");
//          CString sTmp;
//          sTmp.Format("My name is %s", sName);                    // WORKS!
//
//      However if you were to try this with CStdString, your program would
//      crash.
//
//          CStdString sName("Joe");
//          CStdString sTmp;
//          sTmp.Format("My name is %s", sName);                    // CRASHES!
//
//      You must explicitly call c_str() or cast the object to the proper type
//
//          sTmp.Format("My name is %s", sName.c_str());            // WORKS!
//          sTmp.Format("My name is %s", static_cast<PCSTR>(sName));// WORKS!
//          sTmp.Format("My name is %s", (PCSTR)sName);        // WORKS!
//
//      This is because it is illegal to pass anything but a POD type as a
//      variadic argument to a variadic function (i.e. as one of the "..."
//      arguments).  The type const char* is a POD type.  The type CStdString
//      is not.  Of course, neither is the type CString, but CString lets you do
//      it anyway due to the way they laid out the class in binary.  I have no
//      control over this in CStdString since I derive from whatever
//      implementation of basic_string is available.
//
//      However if you have legacy code (which does this) that you want to take
//      out of the MFC world and you don't want to rewrite all your calls to
//      Format(), then you can define this flag and it will no longer crash.
//
//      Note however that this ONLY works for Format(), not sprintf, fprintf,
//      etc.  If you pass a CStdString object to one of those functions, your
//      program will crash.  Not much I can do to get around this, short of
//      writing substitutes for those functions as well.

#define SS_SAFE_FORMAT  // use new template style Format() function


// MACRO: SS_NO_IMPLICIT_CAST
// --------------------------
//      Some people don't like the implicit cast to const char* (or rather to
//      const CT*) that CStdString (and MFC's CString) provide.  That was the
//      whole reason I created this class in the first place, but hey, whatever
//      bakes your cake.  Just #define this macro to get rid of the the implicit
//      cast.

//#define SS_NO_IMPLICIT_CAST // gets rid of operator const CT*()


// MACRO: SS_NO_REFCOUNT
// ---------------------
//    turns off reference counting at the assignment level.  Only needed
//    for the version of basic_string<> that comes with Visual C++ versions
//    6.0 or earlier, and only then in some heavily multithreaded scenarios.
//    Uncomment it if you feel you need it.

//#define SS_NO_REFCOUNT

// MACRO: SS_WIN32
// ---------------
//      When this flag is set, we are building code for the Win32 platform and
//      may use Win32 specific functions (such as LoadString).  This gives us
//      a couple of nice extras for the code.
//
//      Obviously, Microsoft's is not the only compiler available for Win32 out
//      there.  So I can't just check to see if _MSC_VER is defined to detect
//      if I'm building on Win32.  So for now, if you use MS Visual C++ or
//      Borland's compiler, I turn this on.  Otherwise you may turn it on
//      yourself, if you prefer

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(_WIN32)
 #define SS_WIN32
#endif

// MACRO: SS_ANSI
// --------------
//      When this macro is defined, the code attempts only to use ANSI/ISO
//      standard library functions to do it's work.  It will NOT attempt to use
//      any Win32 of Visual C++ specific functions -- even if they are
//      available.  You may define this flag yourself to prevent any Win32
//      of VC++ specific functions from being called.

// If we're not on Win32, we MUST use an ANSI build

#ifndef SS_WIN32
    #if !defined(SS_NO_ANSI)
        #define SS_ANSI
    #endif
#endif

// MACRO: SS_ALLOCA
// ----------------
//      Some implementations of the Standard C Library have a non-standard
//      function known as alloca().  This functions allows one to allocate a
//      variable amount of memory on the stack.  It is needed to implement
//      the ASCII/MBCS conversion macros.
//
//      I wanted to find some way to determine automatically if alloca() is
//    available on this platform via compiler flags but that is asking for
//    trouble.  The crude test presented here will likely need fixing on
//    other platforms.  Therefore I'll leave it up to you to fiddle with
//    this test to determine if it exists.  Just make sure SS_ALLOCA is or
//    is not defined as appropriate and you control this feature.

#if defined(_MSC_VER) && !defined(SS_ANSI)
  #define SS_ALLOCA
#endif


// MACRO: SS_MBCS
// --------------
//    Setting this macro means you are using MBCS characters.  In MSVC builds,
//    this macro gets set automatically by detection of the preprocessor flag
//    _MBCS.  For other platforms you may set it manually if you wish.  The
//    only effect it currently has is to cause the allocation of more space
//    for wchar_t --> char conversions.
//    Note that MBCS does not mean UNICODE.
//
//  #define SS_MBCS
//

#ifdef _MBCS
  #define SS_MBCS
#endif


// MACRO SS_NO_LOCALE
// ------------------
// If your implementation of the Standard C++ Library lacks the <locale> header,
// you can #define this macro to make your code build properly.  Note that this
// is some of my newest code and frankly I'm not very sure of it, though it does
// pass my unit tests.

// #define SS_NO_LOCALE


// Compiler Error regarding _UNICODE and UNICODE
// -----------------------------------------------
// Microsoft header files are screwy.  Sometimes they depend on a preprocessor
// flag named "_UNICODE".  Other times they check "UNICODE" (note the lack of
// leading underscore in the second version".  In several places, they silently
// "synchronize" these two flags this by defining one of the other was defined.
// In older version of this header, I used to try to do the same thing.
//
// However experience has taught me that this is a bad idea.  You get weird
// compiler errors that seem to indicate things like LPWSTR and LPTSTR not being
// equivalent in UNICODE builds, stuff like that (when they MUST be in a proper
// UNICODE  build).  You end up scratching your head and saying, "But that HAS
// to compile!".
//
// So what should you do if you get this error?
//
// Make sure that both macros (_UNICODE and UNICODE) are defined before this
// file is included.  You can do that by either
//
//    a) defining both yourself before any files get included
//    b) including the proper MS headers in the proper order
//    c) including this file before any other file, uncommenting
//       the #defines below, and commenting out the #errors
//
//  Personally I recommend solution a) but it's your call.

#ifdef _MSC_VER
  #if defined (_UNICODE) && !defined (UNICODE)
    #error UNICODE defined  but not UNICODE
  //  #define UNICODE  // no longer silently fix this
  #endif
  #if defined (UNICODE) && !defined (_UNICODE)
    #error Warning, UNICODE defined  but not _UNICODE
  //  #define _UNICODE  // no longer silently fix this
  #endif
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

    // On Win32 we have TCHAR.H so just include it.  This is NOT violating
        // the spirit of SS_ANSI as we are not calling any Win32 functions here.

    #ifdef SS_WIN32

      #include <TCHAR.H>
      #include <WTYPES.H>
      #ifndef STRICT
        #define STRICT
      #endif

        // ... but on non-Win32 platforms, we must #define the types we need.

    #else

      typedef const char*    PCSTR;
      typedef char*      PSTR;
      typedef const wchar_t*  PCWSTR;
      typedef wchar_t*    PWSTR;
      #ifdef UNICODE
        typedef wchar_t    TCHAR;
      #else
        typedef char    TCHAR;
      #endif
      typedef wchar_t      OLECHAR;

    #endif  // #ifndef _WIN32


    // Make sure ASSERT and verify are defined using only ANSI stuff

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

  #else // ...else SS_ANSI is NOT defined

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

#include <string>      // basic_string
#include <algorithm>    // for_each, etc.
#include <functional>    // for StdStringLessNoCase, et al
#ifndef SS_NO_LOCALE
  #include <locale>      // for various facets
#endif

// If this is a recent enough version of VC include comdef.h, so we can write
// member functions to deal with COM types & compiler support classes e.g.
// _bstr_t

#if defined (_MSC_VER) && (_MSC_VER >= 1100)
 #include <comdef.h>
 #define SS_INC_COMDEF  // signal that we #included MS comdef.h file
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
  typedef const TCHAR*      PCTSTR;
  #define PCTSTR_DEFINED
#endif

#if !defined(PCOLESTR) && !defined(PCOLESTR_DEFINED)
  typedef const OLECHAR*      PCOLESTR;
  #define PCOLESTR_DEFINED
#endif

#if !defined(POLESTR) && !defined(POLESTR_DEFINED)
  typedef OLECHAR*        POLESTR;
  #define POLESTR_DEFINED
#endif

#if !defined(PCUSTR) && !defined(PCUSTR_DEFINED)
  typedef const unsigned char*  PCUSTR;
  typedef unsigned char*      PUSTR;
  #define PCUSTR_DEFINED
#endif


// SGI compiler 7.3 doesnt know these  types - oh and btw, remember to use
// -LANG:std in the CXX Flags
#if defined(__sgi)
    typedef unsigned long           DWORD;
    typedef void *                  LPCVOID;
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
  #define schSTR(x)     #x
  #define schSTR2(x)  schSTR(x)
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

    #define SS_USE_FACET(loc, fac) std::_USE(loc, fac)

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

#include <wchar.h>      // Added to Std Library with Amendment #1.

// First define the conversion helper functions.  We define these regardless of
// any preprocessor macro settings since their names won't collide.

// Not sure if we need all these headers.   I believe ANSI says we do.

#include <stdio.h>
#include <stdarg.h>
#include <wctype.h>
#include <ctype.h>
#include <stdlib.h>
#ifndef va_start
  #include <varargs.h>
#endif


#ifdef SS_NO_LOCALE

  #if defined(_WIN32) || defined (_WIN32_WCE)

    inline PWSTR StdCodeCvt(PWSTR pDstW, int nDst, PCSTR pSrcA, int nSrc,
      UINT acp=CP_ACP)
    {
      ASSERT(0 != pSrcA);
      ASSERT(0 != pDstW);
      pDstW[0] = '\0';
      MultiByteToWideChar(acp, 0, pSrcA, nSrc, pDstW, nDst);
      return pDstW;
    }
    inline PWSTR StdCodeCvt(PWSTR pDstW, int nDst, PCUSTR pSrcA, int nSrc,
      UINT acp=CP_ACP)
    {
      return StdCodeCvt(pDstW, nDst, (PCSTR)pSrcA, nSrc, acp);
    }

    inline PSTR StdCodeCvt(PSTR pDstA, int nDst, PCWSTR pSrcW, int nSrc,
      UINT acp=CP_ACP)
    {
      ASSERT(0 != pDstA);
      ASSERT(0 != pSrcW);
      pDstA[0] = '\0';
      WideCharToMultiByte(acp, 0, pSrcW, nSrc, pDstA, nDst, 0, 0);
      return pDstA;
    }
    inline PUSTR StdCodeCvt(PUSTR pDstA, int nDst, PCWSTR pSrcW, int nSrc,
      UINT acp=CP_ACP)
    {
      return (PUSTR)StdCodeCvt((PSTR)pDstA, nDst, pSrcW, nSrc, acp);
    }
  #else
  #endif

#else

  // StdCodeCvt - made to look like Win32 functions WideCharToMultiByte
  //        and MultiByteToWideChar but uses locales in SS_ANSI
  //        builds.  There are a number of overloads.
  //              First argument is the destination buffer.
  //              Second argument is the source buffer
  //#if defined (SS_ANSI) || !defined (SS_WIN32)

  // 'SSCodeCvt' - shorthand name for the codecvt facet we use

  typedef std::codecvt<wchar_t, char, mbstate_t> SSCodeCvt;

  inline PWSTR StdCodeCvt(PWSTR pDstW, int nDst, PCSTR pSrcA, int nSrc,
    const std::locale& loc=std::locale())
  {
    ASSERT(0 != pSrcA);
    ASSERT(0 != pDstW);

    pDstW[0]          = '\0';

    if ( nSrc > 0 )
    {
      PCSTR pNextSrcA      = pSrcA;
      PWSTR pNextDstW      = pDstW;
      SSCodeCvt::result res  = SSCodeCvt::ok;
      const SSCodeCvt& conv  = SS_USE_FACET(loc, SSCodeCvt);
      SSCodeCvt::state_type st= { 0 };
      res            = conv.in(st,
                    pSrcA, pSrcA + nSrc, pNextSrcA,
                    pDstW, pDstW + nDst, pNextDstW);
#ifdef _LINUX
#define ASSERT2(a) if (!(a)) {fprintf(stderr, "StdString: Assertion Failed on line %d\n", __LINE__);}
#else
#define ASSERT2 ASSERT
#endif
      ASSERT2(SSCodeCvt::ok == res);
      ASSERT2(SSCodeCvt::error != res);
      ASSERT2(pNextDstW >= pDstW);
      ASSERT2(pNextSrcA >= pSrcA);
#undef ASSERT2
      // Null terminate the converted string

      if ( pNextDstW - pDstW > nDst )
        *(pDstW + nDst) = '\0';
      else
        *pNextDstW = '\0';
    }
    return pDstW;
  }
  inline PWSTR StdCodeCvt(PWSTR pDstW, int nDst, PCUSTR pSrcA, int nSrc,
    const std::locale& loc=std::locale())
  {
    return StdCodeCvt(pDstW, nDst, (PCSTR)pSrcA, nSrc, loc);
  }

  inline PSTR StdCodeCvt(PSTR pDstA, int nDst, PCWSTR pSrcW, int nSrc,
    const std::locale& loc=std::locale())
  {
    ASSERT(0 != pDstA);
    ASSERT(0 != pSrcW);

    pDstA[0]          = '\0';

    if ( nSrc > 0 )
    {
      PSTR pNextDstA      = pDstA;
      PCWSTR pNextSrcW    = pSrcW;
      SSCodeCvt::result res  = SSCodeCvt::ok;
      const SSCodeCvt& conv  = SS_USE_FACET(loc, SSCodeCvt);
      SSCodeCvt::state_type st= { 0 };
      res            = conv.out(st,
                    pSrcW, pSrcW + nSrc, pNextSrcW,
                    pDstA, pDstA + nDst, pNextDstA);
#ifdef _LINUX
#define ASSERT2(a) if (!(a)) {fprintf(stderr, "StdString: Assertion Failed on line %d\n", __LINE__);}
#else
#define ASSERT2 ASSERT
#endif
      ASSERT2(SSCodeCvt::error != res);
      ASSERT2(SSCodeCvt::ok == res);  // strict, comment out for sanity
      ASSERT2(pNextDstA >= pDstA);
      ASSERT2(pNextSrcW >= pSrcW);
#undef ASSERT2

      // Null terminate the converted string

      if ( pNextDstA - pDstA > nDst )
        *(pDstA + nDst) = '\0';
      else
        *pNextDstA = '\0';
    }
    return pDstA;
  }

  inline PUSTR StdCodeCvt(PUSTR pDstA, int nDst, PCWSTR pSrcW, int nSrc,
    const std::locale& loc=std::locale())
  {
    return (PUSTR)StdCodeCvt((PSTR)pDstA, nDst, pSrcW, nSrc, loc);
  }

#endif



// Unicode/MBCS conversion macros are only available on implementations of
// the "C" library that have the non-standard _alloca function.  As far as I
// know that's only Microsoft's though I've heard that the function exists
// elsewhere.

#if defined(SS_ALLOCA) && !defined SS_NO_CONVERSION

    #include <malloc.h>  // needed for _alloca

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
      #define SSA2W(pa) (\
        ((_pa = pa) == 0) ? 0 : (\
          _cvt = (sslen(_pa)),\
          StdCodeCvt((PWSTR) _alloca((_cvt+1)*2), (_cvt+1)*2, \
              _pa, _cvt, _acp)))
      #define SSW2A(pw) (\
        ((_pw = pw) == 0) ? 0 : (\
          _cvt = sslen(_pw), \
          StdCodeCvt((LPSTR) _alloca((_cvt+1)*2), (_cvt+1)*2, \
          _pw, _cvt, _acp)))
  #else

      #ifndef _DEBUG
        #define SSCVT int _cvt; _cvt; UINT _acp=CP_ACP; _acp;\
           PCWSTR _pw; _pw; PCSTR _pa; _pa
      #else
        #define SSCVT int _cvt = 0; _cvt; UINT _acp=CP_ACP; \
          _acp; PCWSTR _pw=0; _pw; PCSTR _pa=0; _pa
      #endif
      #define SSA2W(pa) (\
        ((_pa = pa) == 0) ? 0 : (\
          _cvt = (sslen(_pa)),\
          StdCodeCvt((PWSTR) _alloca((_cvt+1)*2), (_cvt+1)*2, \
          _pa, _cvt)))
      #define SSW2A(pw) (\
        ((_pw = pw) == 0) ? 0 : (\
          _cvt = (sslen(_pw)),\
          StdCodeCvt((LPSTR) _alloca((_cvt+1)*2), (_cvt+1)*2, \
          _pw, _cvt)))
    #endif

    #define SSA2CW(pa) ((PCWSTR)SSA2W((pa)))
    #define SSW2CA(pw) ((PCSTR)SSW2A((pw)))

    #ifdef UNICODE
      #define SST2A  SSW2A
      #define SSA2T  SSA2W
      #define SST2CA  SSW2CA
      #define SSA2CT  SSA2CW
    // (Did you get a compiler error here about not being able to convert
    // PTSTR into PWSTR?  Then your _UNICODE and UNICODE flags are messed
    // up.  Best bet: #define BOTH macros before including any MS headers.)
      inline PWSTR  SST2W(PTSTR p)      { return p; }
      inline PTSTR  SSW2T(PWSTR p)      { return p; }
      inline PCWSTR  SST2CW(PCTSTR p)    { return p; }
      inline PCTSTR  SSW2CT(PCWSTR p)    { return p; }
    #else
      #define SST2W  SSA2W
      #define SSW2T  SSW2A
      #define SST2CW  SSA2CW
      #define SSW2CT  SSW2CA
      inline PSTR    SST2A(PTSTR p)      { return p; }
      inline PTSTR  SSA2T(PSTR p)      { return p; }
      inline PCSTR  SST2CA(PCTSTR p)    { return p; }
      inline PCTSTR  SSA2CT(PCSTR p)      { return p; }
    #endif // #ifdef UNICODE

    #if defined(UNICODE)
    // in these cases the default (TCHAR) is the same as OLECHAR
      inline PCOLESTR  SST2COLE(PCTSTR p)    { return p; }
      inline PCTSTR  SSOLE2CT(PCOLESTR p)  { return p; }
      inline POLESTR  SST2OLE(PTSTR p)    { return p; }
      inline PTSTR  SSOLE2T(POLESTR p)    { return p; }
    #elif defined(OLE2ANSI)
    // in these cases the default (TCHAR) is the same as OLECHAR
      inline PCOLESTR  SST2COLE(PCTSTR p)    { return p; }
      inline PCTSTR  SSOLE2CT(PCOLESTR p)  { return p; }
      inline POLESTR  SST2OLE(PTSTR p)    { return p; }
      inline PTSTR  SSOLE2T(POLESTR p)    { return p; }
    #else
      //CharNextW doesn't work on Win95 so we use this
      #define SST2COLE(pa)  SSA2CW((pa))
      #define SST2OLE(pa)    SSA2W((pa))
      #define SSOLE2CT(po)  SSW2CA((po))
      #define SSOLE2T(po)    SSW2A((po))
    #endif

    #ifdef OLE2ANSI
      #define SSW2OLE    SSW2A
      #define SSOLE2W    SSA2W
      #define SSW2COLE  SSW2CA
      #define SSOLE2CW  SSA2CW
      inline POLESTR    SSA2OLE(PSTR p)    { return p; }
      inline PSTR      SSOLE2A(POLESTR p)  { return p; }
      inline PCOLESTR    SSA2COLE(PCSTR p)  { return p; }
      inline PCSTR    SSOLE2CA(PCOLESTR p){ return p; }
    #else
      #define SSA2OLE    SSA2W
      #define SSOLE2A    SSW2A
      #define SSA2COLE  SSA2CW
      #define SSOLE2CA  SSW2CA
      inline POLESTR    SSW2OLE(PWSTR p)  { return p; }
      inline PWSTR    SSOLE2W(POLESTR p)  { return p; }
      inline PCOLESTR    SSW2COLE(PCWSTR p)  { return p; }
      inline PCWSTR    SSOLE2CW(PCOLESTR p){ return p; }
    #endif

    // Above we've defined macros that look like MS' but all have
    // an 'SS' prefix.  Now we need the real macros.  We'll either
    // get them from the macros above or from MFC/ATL.

  #if defined (USES_CONVERSION)

    #define _NO_STDCONVERSION  // just to be consistent

  #else

    #ifdef _MFC_VER

      #include <afxconv.h>
      #define _NO_STDCONVERSION // just to be consistent

    #else

      #define USES_CONVERSION SSCVT
      #define A2CW      SSA2CW
      #define W2CA      SSW2CA
      #define T2A        SST2A
      #define A2T        SSA2T
      #define T2W        SST2W
      #define W2T        SSW2T
      #define T2CA      SST2CA
      #define A2CT      SSA2CT
      #define T2CW      SST2CW
      #define W2CT      SSW2CT
      #define ocslen      sslen
      #define ocscpy      sscpy
      #define T2COLE      SST2COLE
      #define OLE2CT      SSOLE2CT
      #define T2OLE      SST2COLE
      #define OLE2T      SSOLE2CT
      #define A2OLE      SSA2OLE
      #define OLE2A      SSOLE2A
      #define W2OLE      SSW2OLE
      #define OLE2W      SSOLE2W
      #define A2COLE      SSA2COLE
      #define OLE2CA      SSOLE2CA
      #define W2COLE      SSW2COLE
      #define OLE2CW      SSOLE2CW

    #endif // #ifdef _MFC_VER
  #endif // #ifndef USES_CONVERSION
#endif // #ifndef SS_NO_CONVERSION

// Define ostring - generic name for std::basic_string<OLECHAR>

#if !defined(ostring) && !defined(OSTRING_DEFINED)
  typedef std::basic_string<OLECHAR> ostring;
  #define OSTRING_DEFINED
#endif

// StdCodeCvt when there's no conversion to be done
template <typename T>
inline T* StdCodeCvt(T* pDst, int nDst, const T* pSrc, int nSrc)
{
  int nChars = SSMIN(nSrc, nDst);

  if ( nChars > 0 )
  {
    pDst[0]        = '\0';
    std::basic_string<T>::traits_type::copy(pDst, pSrc, nChars);
//    std::char_traits<T>::copy(pDst, pSrc, nChars);
    pDst[nChars]  = '\0';
  }

  return pDst;
}
inline PSTR StdCodeCvt(PSTR pDst, int nDst, PCUSTR pSrc, int nSrc)
{
  return StdCodeCvt(pDst, nDst, (PCSTR)pSrc, nSrc);
}
inline PUSTR StdCodeCvt(PUSTR pDst, int nDst, PCSTR pSrc, int nSrc)
{
  return (PUSTR)StdCodeCvt((PSTR)pDst, nDst, pSrc, nSrc);
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
// other words, the preprocessor macro UNICODE is of little help to us in the
// CStdStr template
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
// =============================================================================

#ifdef SS_NO_LOCALE

  // --------------------------------------------------------------------------
  // Win32 GetStringTypeEx wrappers
  // --------------------------------------------------------------------------
  inline bool wsGetStringType(LCID lc, DWORD dwT, PCSTR pS, int nSize,
    WORD* pWd)
  {
    return FALSE != GetStringTypeExA(lc, dwT, pS, nSize, pWd);
  }
  inline bool wsGetStringType(LCID lc, DWORD dwT, PCWSTR pS, int nSize,
    WORD* pWd)
  {
    return FALSE != GetStringTypeExW(lc, dwT, pS, nSize, pWd);
  }


  template<typename CT>
    inline bool ssisspace (CT t)
  {
    WORD toYourMother;
    return  wsGetStringType(GetThreadLocale(), CT_CTYPE1, &t, 1, &toYourMother)
      && 0 != (C1_BLANK & toYourMother);
  }

#endif

// If they defined SS_NO_REFCOUNT, then we must convert all assignments

#if defined (_MSC_VER) && (_MSC_VER < 1300)
  #ifdef SS_NO_REFCOUNT
    #define SSREF(x) (x).c_str()
  #else
    #define SSREF(x) (x)
  #endif
#else
  #define SSREF(x) (x)
#endif

// -----------------------------------------------------------------------------
// sslen: strlen/wcslen wrappers
// -----------------------------------------------------------------------------
template<typename CT> inline int sslen(const CT* pT)
{
  return 0 == pT ? 0 : (int)std::basic_string<CT>::traits_type::length(pT);
//  return 0 == pT ? 0 : std::char_traits<CT>::length(pT);
}
inline SS_NOTHROW int sslen(const std::string& s)
{
  return static_cast<int>(s.length());
}
inline SS_NOTHROW int sslen(const std::wstring& s)
{
  return static_cast<int>(s.length());
}

// -----------------------------------------------------------------------------
// sstolower/sstoupper -- convert characters to upper/lower case
// -----------------------------------------------------------------------------

#ifdef SS_NO_LOCALE
  inline char sstoupper(char ch)    { return (char)::toupper(ch); }
  inline wchar_t sstoupper(wchar_t ch){ return (wchar_t)::towupper(ch); }
  inline char sstolower(char ch)    { return (char)::tolower(ch); }
  inline wchar_t sstolower(wchar_t ch){ return (wchar_t)::tolower(ch); }
#else
  template<typename CT>
  inline CT sstolower(const CT& t, const std::locale& loc = std::locale())
  {
    return std::tolower<CT>(t, loc);
  }
  template<typename CT>
  inline CT sstoupper(const CT& t, const std::locale& loc = std::locale())
  {
    return std::toupper<CT>(t, loc);
  }
#endif

// -----------------------------------------------------------------------------
// ssasn: assignment functions -- assign "sSrc" to "sDst"
// -----------------------------------------------------------------------------
typedef std::string::size_type    SS_SIZETYPE; // just for shorthand, really
typedef std::string::pointer    SS_PTRTYPE;
typedef std::wstring::size_type    SW_SIZETYPE;
typedef std::wstring::pointer    SW_PTRTYPE;


template <typename T>
inline void  ssasn(std::basic_string<T>& sDst, const std::basic_string<T>& sSrc)
{
  if ( sDst.c_str() != sSrc.c_str() )
  {
    sDst.erase();
    sDst.assign(SSREF(sSrc));
  }
}
template <typename T>
inline void  ssasn(std::basic_string<T>& sDst, const T *pA)
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
    sDst =sDst.substr(static_cast<typename std::basic_string<T>::size_type>(pA-sDst.c_str()));
  }

  // Otherwise (most cases) apply the assignment bug fix, if applicable
  // and do the assignment

  else
  {
    Q172398(sDst);
    sDst.assign(pA);
  }
}
inline void  ssasn(std::string& sDst, const std::wstring& sSrc)
{
  if ( sSrc.empty() )
  {
    sDst.erase();
  }
  else
  {
    int nDst  = static_cast<int>(sSrc.size());

    // In MBCS builds, pad the buffer to account for the possibility of
    // some 3 byte characters.  Not perfect but should get most cases.

#ifdef SS_MBCS
    // In MBCS builds, we don't know how long the destination string will be.
    nDst  = static_cast<int>(static_cast<double>(nDst) * 1.3);
    sDst.resize(nDst+1);
    PCSTR szCvt = StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()), nDst,
      sSrc.c_str(), static_cast<int>(sSrc.size()));
    sDst.resize(sslen(szCvt));
#else
    sDst.resize(nDst+1);
    StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()), nDst,
      sSrc.c_str(), static_cast<int>(sSrc.size()));
    sDst.resize(sSrc.size());
#endif
  }
}
inline void  ssasn(std::string& sDst, PCWSTR pW)
{
  int nSrc  = sslen(pW);
  if ( nSrc > 0 )
  {
    int nSrc  = sslen(pW);
    int nDst  = nSrc;

    // In MBCS builds, pad the buffer to account for the possibility of
    // some 3 byte characters.  Not perfect but should get most cases.

#ifdef SS_MBCS
    nDst  = static_cast<int>(static_cast<double>(nDst) * 1.3);
    // In MBCS builds, we don't know how long the destination string will be.
    sDst.resize(nDst + 1);
    PCSTR szCvt = StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()), nDst,
      pW, nSrc);
    sDst.resize(sslen(szCvt));
#else
    sDst.resize(nDst + 1);
    StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()), nDst, pW, nSrc);
    sDst.resize(nDst);
#endif
  }
  else
  {
    sDst.erase();
  }
}
inline void ssasn(std::string& sDst, const int nNull)
{
  //UNUSED(nNull);
  ASSERT(nNull==0);
  sDst.assign("");
}
#undef StrSizeType
inline void  ssasn(std::wstring& sDst, const std::string& sSrc)
{
  if ( sSrc.empty() )
  {
    sDst.erase();
  }
  else
  {
    int nSrc  = static_cast<int>(sSrc.size());
    int nDst  = nSrc;

    sDst.resize(nSrc+1);
    PCWSTR szCvt = StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()), nDst,
      sSrc.c_str(), nSrc);

    sDst.resize(sslen(szCvt));
  }
}
inline void  ssasn(std::wstring& sDst, PCSTR pA)
{
  int nSrc  = sslen(pA);

  if ( 0 == nSrc )
  {
    sDst.erase();
  }
  else
  {
    int nDst  = nSrc;
    sDst.resize(nDst+1);
    PCWSTR szCvt = StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()), nDst, pA,
      nSrc);

    sDst.resize(sslen(szCvt));
  }
}
inline void ssasn(std::wstring& sDst, const int nNull)
{
  //UNUSED(nNull);
  ASSERT(nNull==0);
  sDst.assign(L"");
}

// -----------------------------------------------------------------------------
// ssadd: string object concatenation -- add second argument to first
// -----------------------------------------------------------------------------
inline void  ssadd(std::string& sDst, const std::wstring& sSrc)
{
  int nSrc  = static_cast<int>(sSrc.size());

  if ( nSrc > 0 )
  {
    int nDst  = static_cast<int>(sDst.size());
    int nAdd  = nSrc;

    // In MBCS builds, pad the buffer to account for the possibility of
    // some 3 byte characters.  Not perfect but should get most cases.

#ifdef SS_MBCS
    nAdd    = static_cast<int>(static_cast<double>(nAdd) * 1.3);
    sDst.resize(nDst+nAdd+1);
    PCSTR szCvt = StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()+nDst),
      nAdd, sSrc.c_str(), nSrc);
    sDst.resize(nDst + sslen(szCvt));
#else
    sDst.resize(nDst+nAdd+1);
    StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()+nDst), nAdd, sSrc.c_str(), nSrc);
    sDst.resize(nDst + nAdd);
#endif
  }
}
template <typename T>
inline void  ssadd(typename std::basic_string<T>& sDst, const typename std::basic_string<T>& sSrc)
{
  sDst += sSrc;
}
inline void  ssadd(std::string& sDst, PCWSTR pW)
{
  int nSrc    = sslen(pW);
  if ( nSrc > 0 )
  {
    int nDst  = static_cast<int>(sDst.size());
    int nAdd  = nSrc;

#ifdef SS_MBCS
    nAdd  = static_cast<int>(static_cast<double>(nAdd) * 1.3);
    sDst.resize(nDst + nAdd + 1);
    PCSTR szCvt = StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()+nDst),
      nAdd, pW, nSrc);
    sDst.resize(nDst + sslen(szCvt));
#else
    sDst.resize(nDst + nAdd + 1);
    StdCodeCvt(const_cast<SS_PTRTYPE>(sDst.data()+nDst), nAdd, pW, nSrc);
    sDst.resize(nDst + nSrc);
#endif
  }
}
template <typename T>
inline void  ssadd(typename std::basic_string<T>& sDst, const T *pA)
{
  if ( pA )
  {
    // If the string being added is our internal string or a part of our
    // internal string, then we must NOT do any reallocation without
    // first copying that string to another object (since we're using a
    // direct pointer)

    if ( pA >= sDst.c_str() && pA <= sDst.c_str()+sDst.length())
    {
      if ( sDst.capacity() <= sDst.size()+sslen(pA) )
        sDst.append(std::basic_string<T>(pA));
      else
        sDst.append(pA);
    }
    else
    {
      sDst.append(pA);
    }
  }
}
inline void  ssadd(std::wstring& sDst, const std::string& sSrc)
{
  if ( !sSrc.empty() )
  {
    int nSrc  = static_cast<int>(sSrc.size());
    int nDst  = static_cast<int>(sDst.size());

    sDst.resize(nDst + nSrc + 1);
#ifdef SS_MBCS
    PCWSTR szCvt = StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()+nDst),
      nSrc, sSrc.c_str(), nSrc+1);
    sDst.resize(nDst + sslen(szCvt));
#else
    StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()+nDst), nSrc, sSrc.c_str(), nSrc+1);
    sDst.resize(nDst + nSrc);
#endif
  }
}
inline void  ssadd(std::wstring& sDst, PCSTR pA)
{
  int nSrc    = sslen(pA);

  if ( nSrc > 0 )
  {
    int nDst  = static_cast<int>(sDst.size());

    sDst.resize(nDst + nSrc + 1);
#ifdef SS_MBCS
    PCWSTR szCvt = StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()+nDst),
      nSrc, pA, nSrc+1);
    sDst.resize(nDst + sslen(szCvt));
#else
    StdCodeCvt(const_cast<SW_PTRTYPE>(sDst.data()+nDst), nSrc, pA, nSrc+1);
    sDst.resize(nDst + nSrc);
#endif
  }
}

// -----------------------------------------------------------------------------
// sscmp: comparison (case sensitive, not affected by locale)
// -----------------------------------------------------------------------------
template<typename CT>
inline int sscmp(const CT* pA1, const CT* pA2)
{
    CT f;
    CT l;

    do
    {
      f = *(pA1++);
      l = *(pA2++);
    } while ( (f) && (f == l) );

    return (int)(f - l);
}

// -----------------------------------------------------------------------------
// ssicmp: comparison (case INsensitive, not affected by locale)
// -----------------------------------------------------------------------------
template<typename CT>
inline int ssicmp(const CT* pA1, const CT* pA2)
{
  // Using the "C" locale = "not affected by locale"

  std::locale loc = std::locale::classic();
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

// -----------------------------------------------------------------------------
// ssupr/sslwr: Uppercase/Lowercase conversion functions
// -----------------------------------------------------------------------------

template<typename CT>
inline void sslwr(CT* pT, size_t nLen, const std::locale& loc=std::locale())
{
  SS_USE_FACET(loc, std::ctype<CT>).tolower(pT, pT+nLen);
}
template<typename CT>
inline void ssupr(CT* pT, size_t nLen, const std::locale& loc=std::locale())
{
  SS_USE_FACET(loc, std::ctype<CT>).toupper(pT, pT+nLen);
}

// -----------------------------------------------------------------------------
// vsprintf/vswprintf or _vsnprintf/_vsnwprintf equivalents.  In standard
// builds we can't use _vsnprintf/_vsnwsprintf because they're MS extensions.
//
// -----------------------------------------------------------------------------
// Borland's headers put some ANSI "C" functions in the 'std' namespace.
// Promote them to the global namespace so we can use them here.

#if defined(__BORLANDC__)
    using std::vsprintf;
    using std::vswprintf;
#endif

  // GNU is supposed to have vsnprintf and vsnwprintf.  But only the newer
  // distributions do.

#if defined(__GNUC__)

  inline int ssvsprintf(PSTR pA, size_t nCount, PCSTR pFmtA, va_list vl)
  {
    return vsnprintf(pA, nCount, pFmtA, vl);
  }
  inline int ssvsprintf(PWSTR pW, size_t nCount, PCWSTR pFmtW, va_list vl)
  {
    return vswprintf(pW, nCount, pFmtW, vl);
  }

  // Microsofties can use
#elif defined(_MSC_VER) && !defined(SS_ANSI)

  inline int  ssvsprintf(PSTR pA, size_t nCount, PCSTR pFmtA, va_list vl)
  {
    return _vsnprintf(pA, nCount, pFmtA, vl);
  }
  inline int  ssvsprintf(PWSTR pW, size_t nCount, PCWSTR pFmtW, va_list vl)
  {
    return _vsnwprintf(pW, nCount, pFmtW, vl);
  }

#elif defined (SS_DANGEROUS_FORMAT)  // ignore buffer size parameter if needed?

  inline int ssvsprintf(PSTR pA, size_t /*nCount*/, PCSTR pFmtA, va_list vl)
  {
    return vsprintf(pA, pFmtA, vl);
  }

  inline int ssvsprintf(PWSTR pW, size_t nCount, PCWSTR pFmtW, va_list vl)
  {
    // JMO: Some distributions of the "C" have a version of vswprintf that
        // takes 3 arguments (e.g. Microsoft, Borland, GNU).  Others have a
        // version which takes 4 arguments (an extra "count" argument in the
        // second position.  The best stab I can take at this so far is that if
        // you are NOT running with MS, Borland, or GNU, then I'll assume you
        // have the version that takes 4 arguments.
        //
        // I'm sure that these checks don't catch every platform correctly so if
        // you get compiler errors on one of the lines immediately below, it's
        // probably because your implemntation takes a different number of
        // arguments.  You can comment out the offending line (and use the
        // alternate version) or you can figure out what compiler flag to check
        // and add that preprocessor check in.  Regardless, if you get an error
        // on these lines, I'd sure like to hear from you about it.
        //
        // Thanks to Ronny Schulz for the SGI-specific checks here.

//  #if !defined(__MWERKS__) && !defined(__SUNPRO_CC_COMPAT) && !defined(__SUNPRO_CC)
    #if    !defined(_MSC_VER) \
        && !defined (__BORLANDC__) \
        && !defined(__GNUC__) \
        && !defined(__sgi)

        return vswprintf(pW, nCount, pFmtW, vl);

    // suddenly with the current SGI 7.3 compiler there is no such function as
    // vswprintf and the substitute needs explicit casts to compile

    #elif defined(__sgi)

        nCount;
        return vsprintf( (char *)pW, (char *)pFmtW, vl);

    #else

        nCount;
        return vswprintf(pW, pFmtW, vl);

    #endif

  }

#endif

  // GOT COMPILER PROBLEMS HERE?
  // ---------------------------
  // Does your compiler choke on one or more of the following 2 functions?  It
  // probably means that you don't have have either vsnprintf or vsnwprintf in
  // your version of the CRT.  This is understandable since neither is an ANSI
  // "C" function.  However it still leaves you in a dilemma.  In order to make
  // this code build, you're going to have to to use some non-length-checked
  // formatting functions that every CRT has:  vsprintf and vswprintf.
  //
  // This is very dangerous.  With the proper erroneous (or malicious) code, it
  // can lead to buffer overlows and crashing your PC.  Use at your own risk
  // In order to use them, just #define SS_DANGEROUS_FORMAT at the top of
  // this file.
  //
  // Even THEN you might not be all the way home due to some non-conforming
  // distributions.  More on this in the comments below.

  inline int  ssnprintf(PSTR pA, size_t nCount, PCSTR pFmtA, va_list vl)
  {
  #ifdef _MSC_VER
      return _vsnprintf(pA, nCount, pFmtA, vl);
  #else
      return vsnprintf(pA, nCount, pFmtA, vl);
  #endif
  }
  inline int  ssnprintf(PWSTR pW, size_t nCount, PCWSTR pFmtW, va_list vl)
  {
  #ifdef _MSC_VER
      return _vsnwprintf(pW, nCount, pFmtW, vl);
  #else
      return vswprintf(pW, nCount, pFmtW, vl);
  #endif
  }




// -----------------------------------------------------------------------------
// ssload: Type safe, overloaded ::LoadString wrappers
// There is no equivalent of these in non-Win32-specific builds.  However, I'm
// thinking that with the message facet, there might eventually be one
// -----------------------------------------------------------------------------
#if defined (SS_WIN32) && !defined(SS_ANSI)
  inline int ssload(HMODULE hInst, UINT uId, PSTR pBuf, int nMax)
  {
    return ::LoadStringA(hInst, uId, pBuf, nMax);
  }
  inline int ssload(HMODULE hInst, UINT uId, PWSTR pBuf, int nMax)
  {
    return ::LoadStringW(hInst, uId, pBuf, nMax);
  }
#if defined ( _MSC_VER ) && ( _MSC_VER >= 1500 )
  inline int ssload(HMODULE hInst, UINT uId, uint16_t *pBuf, int nMax)
  {
    return 0;
  }
  inline int ssload(HMODULE hInst, UINT uId, uint32_t *pBuf, int nMax)
  {
    return 0;
  }
#endif
#endif


// -----------------------------------------------------------------------------
// sscoll/ssicoll: Collation wrappers
//    Note -- with MSVC I have reversed the arguments order here because the
//    functions appear to return the opposite of what they should
// -----------------------------------------------------------------------------
#ifndef SS_NO_LOCALE
template <typename CT>
inline int sscoll(const CT* sz1, int nLen1, const CT* sz2, int nLen2)
{
  const std::collate<CT>& coll =
    SS_USE_FACET(std::locale(), std::collate<CT>);

  return coll.compare(sz2, sz2+nLen2, sz1, sz1+nLen1);
}
template <typename CT>
inline int ssicoll(const CT* sz1, int nLen1, const CT* sz2, int nLen2)
{
  const std::locale loc;
  const std::collate<CT>& coll = SS_USE_FACET(loc, std::collate<CT>);

  // Some implementations seem to have trouble using the collate<>
  // facet typedefs so we'll just default to basic_string and hope
  // that's what the collate facet uses (which it generally should)

//  std::collate<CT>::string_type s1(sz1);
//  std::collate<CT>::string_type s2(sz2);
  const std::basic_string<CT> sEmpty;
    std::basic_string<CT> s1(sz1 ? sz1 : sEmpty.c_str());
    std::basic_string<CT> s2(sz2 ? sz2 : sEmpty.c_str());

  sslwr(const_cast<CT*>(s1.c_str()), nLen1, loc);
  sslwr(const_cast<CT*>(s2.c_str()), nLen2, loc);
  return coll.compare(s2.c_str(), s2.c_str()+nLen2,
            s1.c_str(), s1.c_str()+nLen1);
}
#endif


// -----------------------------------------------------------------------------
// ssfmtmsg: FormatMessage equivalents.  Needed because I added a CString facade
// Again -- no equivalent of these on non-Win32 builds but their might one day
// be one if the message facet gets implemented
// -----------------------------------------------------------------------------
#if defined (SS_WIN32) && !defined(SS_ANSI)
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
#else
#endif



// FUNCTION: sscpy.  Copies up to 'nMax' characters from pSrc to pDst.
// -----------------------------------------------------------------------------
// FUNCTION:  sscpy
//    inline int sscpy(PSTR pDst, PCSTR pSrc, int nMax=-1);
//    inline int sscpy(PUSTR pDst,  PCSTR pSrc, int nMax=-1)
//    inline int sscpy(PSTR pDst, PCWSTR pSrc, int nMax=-1);
//    inline int sscpy(PWSTR pDst, PCWSTR pSrc, int nMax=-1);
//    inline int sscpy(PWSTR pDst, PCSTR pSrc, int nMax=-1);
//
// DESCRIPTION:
//    This function is very much (but not exactly) like strcpy.  These
//    overloads simplify copying one C-style string into another by allowing
//    the caller to specify two different types of strings if necessary.
//
//    The strings must NOT overlap
//
//    "Character" is expressed in terms of the destination string, not
//    the source.  If no 'nMax' argument is supplied, then the number of
//    characters copied will be sslen(pSrc).  A NULL terminator will
//    also be added so pDst must actually be big enough to hold nMax+1
//    characters.  The return value is the number of characters copied,
//    not including the NULL terminator.
//
// PARAMETERS:
//    pSrc - the string to be copied FROM.  May be a char based string, an
//         MBCS string (in Win32 builds) or a wide string (wchar_t).
//    pSrc - the string to be copied TO.  Also may be either MBCS or wide
//    nMax - the maximum number of characters to be copied into szDest.  Note
//         that this is expressed in whatever a "character" means to pDst.
//         If pDst is a wchar_t type string than this will be the maximum
//         number of wchar_ts that my be copied.  The pDst string must be
//         large enough to hold least nMaxChars+1 characters.
//         If the caller supplies no argument for nMax this is a signal to
//         the routine to copy all the characters in pSrc, regardless of
//         how long it is.
//
// RETURN VALUE: none
// -----------------------------------------------------------------------------

template<typename CT1, typename CT2>
inline int sscpycvt(CT1* pDst, const CT2* pSrc, int nMax)
{
  // Note -- we assume pDst is big enough to hold pSrc.  If not, we're in
  // big trouble.  No bounds checking.  Caveat emptor.

  int nSrc = sslen(pSrc);

  const CT1* szCvt = StdCodeCvt(pDst, nMax, pSrc, nSrc);

  // If we're copying the same size characters, then all the "code convert"
  // just did was basically memcpy so the #of characters copied is the same
  // as the number requested.  I should probably specialize this function
  // template to achieve this purpose as it is silly to do a runtime check
  // of a fact known at compile time.  I'll get around to it.

  return sslen(szCvt);
}

template<typename T>
inline int sscpycvt(T* pDst, const T* pSrc, int nMax)
{
  int nCount = nMax;
  for (; nCount > 0 && *pSrc; ++pSrc, ++pDst, --nCount)
    std::basic_string<T>::traits_type::assign(*pDst, *pSrc);

  *pDst = 0;
  return nMax - nCount;
}

inline int sscpycvt(PWSTR pDst, PCSTR pSrc, int nMax)
{
  // Note -- we assume pDst is big enough to hold pSrc.  If not, we're in
  // big trouble.  No bounds checking.  Caveat emptor.

  const PWSTR szCvt = StdCodeCvt(pDst, nMax, pSrc, nMax);
  return sslen(szCvt);
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
    return sscpycvt(pDst, static_cast<PCOLESTR>(bs),
            SSMIN(nMax, static_cast<int>(bs.length())));
  }
  template<typename CT1>
  inline int sscpy(CT1* pDst, const _bstr_t& bs)
  {
    return sscpy(pDst, bs, static_cast<int>(bs.length()));
  }
#endif


// -----------------------------------------------------------------------------
// Functional objects for changing case.  They also let you pass locales
// -----------------------------------------------------------------------------

#ifdef SS_NO_LOCALE
  template<typename CT>
  struct SSToUpper : public std::unary_function<CT, CT>
  {
    inline CT operator()(const CT& t) const
    {
      return sstoupper(t);
    }
  };
  template<typename CT>
  struct SSToLower : public std::unary_function<CT, CT>
  {
    inline CT operator()(const CT& t) const
    {
      return sstolower(t);
    }
  };
#else
  template<typename CT>
  struct SSToUpper : public std::binary_function<CT, std::locale, CT>
  {
    inline CT operator()(const CT& t, const std::locale& loc) const
    {
      return sstoupper<CT>(t, loc);
    }
  };
  template<typename CT>
  struct SSToLower : public std::binary_function<CT, std::locale, CT>
  {
    inline CT operator()(const CT& t, const std::locale& loc) const
    {
      return sstolower<CT>(t, loc);
    }
  };
#endif

// This struct is used for TrimRight() and TrimLeft() function implementations.
//template<typename CT>
//struct NotSpace : public std::unary_function<CT, bool>
//{
//  const std::locale& loc;
//  inline NotSpace(const std::locale& locArg) : loc(locArg) {}
//  inline bool operator() (CT t) { return !std::isspace(t, loc); }
//};
template<typename CT>
struct NotSpace : public std::unary_function<CT, bool>
{
  // DINKUMWARE BUG:
  // Note -- using std::isspace in a COM DLL gives us access violations
  // because it causes the dynamic addition of a function to be called
  // when the library shuts down.  Unfortunately the list is maintained
  // in DLL memory but the function is in static memory.  So the COM DLL
  // goes away along with the function that was supposed to be called,
  // and then later when the DLL CRT shuts down it unloads the list and
  // tries to call the long-gone function.
  // This is DinkumWare's implementation problem.  If you encounter this
  // problem, you may replace the calls here with good old isspace() and
  // iswspace() from the CRT unless they specify SS_ANSI

#ifdef SS_NO_LOCALE

  bool operator() (CT t) const { return !ssisspace(t); }

#else
  const std::locale loc;
  NotSpace(const std::locale& locArg=std::locale()) : loc(locArg) {}
  bool operator() (CT t) const { return !std::isspace(t, loc); }
#endif
};




//      Now we can define the template (finally!)
// =============================================================================
// TEMPLATE: CStdStr
//    template<typename CT> class CStdStr : public std::basic_string<CT>
//
// REMARKS:
//    This template derives from basic_string<CT> and adds some MFC CString-
//    like functionality
//
//    Basically, this is my attempt to make Standard C++ library strings as
//    easy to use as the MFC CString class.
//
//    Note that although this is a template, it makes the assumption that the
//    template argument (CT, the character type) is either char or wchar_t.
// =============================================================================

//#define CStdStr _SS  // avoid compiler warning 4786

//    template<typename ARG> ARG& FmtArg(ARG& arg)  { return arg; }
//    PCSTR  FmtArg(const std::string& arg)  { return arg.c_str(); }
//    PCWSTR FmtArg(const std::wstring& arg) { return arg.c_str(); }

template<typename ARG>
struct FmtArg
{
    explicit FmtArg(const ARG& arg) : a_(arg) {}
    const ARG& operator()() const { return a_; }
    const ARG& a_;
private:
    FmtArg& operator=(const FmtArg&) { return *this; }
};

template<typename CT>
class CStdStr : public std::basic_string<CT>
{
  // Typedefs for shorter names.  Using these names also appears to help
  // us avoid some ambiguities that otherwise arise on some platforms

  #define MYBASE std::basic_string<CT>         // my base class
  //typedef typename std::basic_string<CT>    MYBASE;   // my base class
  typedef CStdStr<CT>              MYTYPE;   // myself
  typedef typename MYBASE::const_pointer    PCMYSTR; // PCSTR or PCWSTR
  typedef typename MYBASE::pointer      PMYSTR;   // PSTR or PWSTR
  typedef typename MYBASE::iterator      MYITER;  // my iterator type
  typedef typename MYBASE::const_iterator    MYCITER; // you get the idea...
  typedef typename MYBASE::reverse_iterator  MYRITER;
  typedef typename MYBASE::size_type      MYSIZE;
  typedef typename MYBASE::value_type      MYVAL;
  typedef typename MYBASE::allocator_type    MYALLOC;

public:
  // shorthand conversion from PCTSTR to string resource ID
  #define SSRES(pctstr)  LOWORD(reinterpret_cast<unsigned long>(pctstr))

  bool TryLoad(const void* pT)
  {
    bool bLoaded = false;

#if defined(SS_WIN32) && !defined(SS_ANSI)
    if ( ( pT != NULL ) && SS_IS_INTRESOURCE(pT) )
    {
      UINT nId = LOWORD(reinterpret_cast<unsigned long>(pT));
      if ( !LoadString(nId) )
      {
        TRACE(_T("Can't load string %u\n"), SSRES(pT));
      }
      bLoaded = true;
    }
#endif

    return bLoaded;
  }


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

#ifdef SS_UNSIGNED
  CStdStr(PCUSTR pU)
  {
    *this = reinterpret_cast<PCSTR>(pU);
  }
#endif

  CStdStr(PCSTR pA)
  {
  #ifdef SS_ANSI
    *this = pA;
  #else
    if ( !TryLoad(pA) )
      *this = pA;
  #endif
  }

  CStdStr(PCWSTR pW)
  {
  #ifdef SS_ANSI
    *this = pW;
  #else
    if ( !TryLoad(pW) )
      *this = pW;
  #endif
  }

  CStdStr(uint16_t* pW)
  {
  #ifdef SS_ANSI
    *this = pW;
  #else
    if ( !TryLoad(pW) )
      *this = pW;
  #endif
  }

  CStdStr(uint32_t* pW)
  {
  #ifdef SS_ANSI
    *this = pW;
  #else
    if ( !TryLoad(pW) )
      *this = pW;
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
        this->append(static_cast<PCMYSTR>(bstr), bstr.length());
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

#ifdef SS_UNSIGNED
  MYTYPE& operator=(PCUSTR pU)
  {
    ssasn(*this, reinterpret_cast<PCSTR>(pU));
    return *this;
  }
#endif

  MYTYPE& operator=(uint16_t* pA)
  {
    ssasn(*this, pA);
    return *this;
  }

  MYTYPE& operator=(uint32_t* pA)
  {
    ssasn(*this, pA);
    return *this;
  }

  MYTYPE& operator=(CT t)
  {
    Q172398(*this);
    this->assign(1, t);
    return *this;
  }

  #ifdef SS_INC_COMDEF
    MYTYPE& operator=(const _bstr_t& bstr)
    {
      if ( bstr.length() > 0 )
      {
        this->assign(static_cast<PCMYSTR>(bstr), bstr.length());
        return *this;
      }
      else
      {
        this->erase();
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
      Q172398(*this);
      sscpy(GetBuffer(str.size()+1), SSREF(str));
      this->ReleaseBuffer(str.size());
      return *this;
    }

    MYTYPE& assign(const MYTYPE& str, MYSIZE nStart, MYSIZE nChars)
    {
      // This overload of basic_string::assign is supposed to assign up to
      // <nChars> or the NULL terminator, whichever comes first.  Since we
      // are about to call a less forgiving overload (in which <nChars>
      // must be a valid length), we must adjust the length here to a safe
      // value.  Thanks to Ullrich Poll�hne for catching this bug

      nChars    = SSMIN(nChars, str.length() - nStart);
      MYTYPE strTemp(str.c_str()+nStart, nChars);
      Q172398(*this);
      this->assign(strTemp);
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
      // value. Thanks to Ullrich Poll�hne for catching this bug

      nChars    = SSMIN(nChars, str.length() - nStart);

      // Watch out for assignment to self

      if ( this == &str )
      {
        MYTYPE strTemp(str.c_str() + nStart, nChars);
        static_cast<MYBASE*>(this)->assign(strTemp);
      }
      else
      {
        Q172398(*this);
        static_cast<MYBASE*>(this)->assign(str.c_str()+nStart, nChars);
      }
      return *this;
    }

    MYTYPE& assign(const CT* pC, MYSIZE nChars)
    {
      // Q172398 only fix -- erase before assigning, but not if we're
      // assigning from our own buffer

  #if defined ( _MSC_VER ) && ( _MSC_VER < 1200 )
      if ( !this->empty() &&
        ( pC < this->data() || pC > this->data() + this->capacity() ) )
      {
        this->erase();
      }
  #endif
      Q172398(*this);
      static_cast<MYBASE*>(this)->assign(pC, nChars);
      return *this;
    }

    MYTYPE& assign(MYSIZE nChars, MYVAL val)
    {
      Q172398(*this);
      static_cast<MYBASE*>(this)->assign(nChars, val);
      return *this;
    }

    MYTYPE& assign(const CT* pT)
    {
      return this->assign(pT, MYBASE::traits_type::length(pT));
    }

    MYTYPE& assign(MYCITER iterFirst, MYCITER iterLast)
    {
  #if defined ( _MSC_VER ) && ( _MSC_VER < 1200 )
      // Q172398 fix.  don't call erase() if we're assigning from ourself
      if ( iterFirst < this->begin() ||
                 iterFirst > this->begin() + this->size() )
            {
        this->erase()
            }
  #endif
      this->replace(this->begin(), this->end(), iterFirst, iterLast);
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

  MYTYPE& operator+=(uint16_t* pW)
  {
    ssadd(*this, pW);
    return *this;
  }

  MYTYPE& operator+=(uint32_t* pW)
  {
    ssadd(*this, pW);
    return *this;
  }

  MYTYPE& operator+=(CT t)
  {
    this->append(1, t);
    return *this;
  }
  #ifdef SS_INC_COMDEF  // if we have _bstr_t, define a += for it too.
    MYTYPE& operator+=(const _bstr_t& bstr)
    {
      return this->operator+=(static_cast<PCMYSTR>(bstr));
    }
  #endif


  // -------------------------------------------------------------------------
  // Case changing functions
  // -------------------------------------------------------------------------

    MYTYPE& ToUpper(const std::locale& loc=std::locale())
  {
    // Note -- if there are any MBCS character sets in which the lowercase
    // form a character takes up a different number of bytes than the
    // uppercase form, this would probably not work...

    std::transform(this->begin(),
             this->end(),
             this->begin(),
#ifdef SS_NO_LOCALE
             SSToUpper<CT>());
#else
             std::bind2nd(SSToUpper<CT>(), loc));
#endif

    // ...but if it were, this would probably work better.  Also, this way
    // seems to be a bit faster when anything other then the "C" locale is
    // used...

//    if ( !empty() )
//    {
//      ssupr(this->GetBuf(), this->size(), loc);
//      this->RelBuf();
//    }

    return *this;
  }

  MYTYPE& ToLower(const std::locale& loc=std::locale())
  {
    // Note -- if there are any MBCS character sets in which the lowercase
    // form a character takes up a different number of bytes than the
    // uppercase form, this would probably not work...

    std::transform(this->begin(),
             this->end(),
             this->begin(),
#ifdef SS_NO_LOCALE
             SSToLower<CT>());
#else
             std::bind2nd(SSToLower<CT>(), loc));
#endif

    // ...but if it were, this would probably work better.  Also, this way
    // seems to be a bit faster when anything other then the "C" locale is
    // used...

//    if ( !empty() )
//    {
//      sslwr(this->GetBuf(), this->size(), loc);
//      this->RelBuf();
//    }
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
    // In VC 7 and later, of course, the ref-counting stuff is gone.
  // -------------------------------------------------------------------------

  CT* GetBuf(int nMinLen=-1)
  {
    if ( static_cast<int>(this->size()) < nMinLen )
      this->resize(static_cast<MYSIZE>(nMinLen));

    return this->empty() ? const_cast<CT*>(this->data()) : &(this->at(0));
  }

  CT* SetBuf(int nLen)
  {
    nLen = ( nLen > 0 ? nLen : 0 );
    if ( this->capacity() < 1 && nLen == 0 )
      this->resize(1);

    this->resize(static_cast<MYSIZE>(nLen));
    return const_cast<CT*>(this->data());
  }
  void RelBuf(int nNewLen=-1)
  {
    this->resize(static_cast<MYSIZE>(nNewLen > -1 ? nNewLen :
                                                        sslen(this->c_str())));
  }

  void BufferRel()     { RelBuf(); }      // backwards compatability
  CT*  Buffer()       { return GetBuf(); }  // backwards compatability
  CT*  BufferSet(int nLen) { return SetBuf(nLen);}// backwards compatability

  bool Equals(const CT* pT, bool bUseCase=false) const
  {
    return  0 == (bUseCase ? this->compare(pT) : ssicmp(this->c_str(), pT));
  }

  // -------------------------------------------------------------------------
  // FUNCTION:  CStdStr::Load
  // REMARKS:
  //    Loads string from resource specified by nID
  //
  // PARAMETERS:
  //    nID - resource Identifier.  Purely a Win32 thing in this case
  //
  // RETURN VALUE:
  //    true if successful, false otherwise
  // -------------------------------------------------------------------------

#ifndef SS_ANSI

  bool Load(UINT nId, HMODULE hModule=NULL)
  {
    bool bLoaded    = false;  // set to true of we succeed.

  #ifdef _MFC_VER    // When in Rome (or MFC land)...

    // If they gave a resource handle, use it.  Note - this is archaic
    // and not really what I would recommend.  But then again, in MFC
    // land, you ought to be using CString for resources anyway since
    // it walks the resource chain for you.

    HMODULE hModuleOld = NULL;

    if ( NULL != hModule )
    {
      hModuleOld = AfxGetResourceHandle();
      AfxSetResourceHandle(hModule);
    }

    // ...load the string

    CString strRes;
    bLoaded        = FALSE != strRes.LoadString(nId);

    // ...and if we set the resource handle, restore it.

    if ( NULL != hModuleOld )
      AfxSetResourceHandle(hModule);

    if ( bLoaded )
      *this      = strRes;

  #else // otherwise make our own hackneyed version of CString's Load

    // Get the resource name and module handle

    if ( NULL == hModule )
      hModule      = GetResourceHandle();

    PCTSTR szName    = MAKEINTRESOURCE((nId>>4)+1); // lifted
    DWORD dwSize    = 0;

    // No sense continuing if we can't find the resource

    HRSRC hrsrc      = ::FindResource(hModule, szName, RT_STRING);

    if ( NULL == hrsrc )
    {
      TRACE(_T("Cannot find resource %d: 0x%X"), nId, ::GetLastError());
    }
    else if ( 0 == (dwSize = ::SizeofResource(hModule, hrsrc) / sizeof(CT)))
    {
      TRACE(_T("Cant get size of resource %d 0x%X\n"),nId,GetLastError());
    }
    else
    {
      bLoaded      = 0 != ssload(hModule, nId, GetBuf(dwSize), dwSize);
      ReleaseBuffer();
    }

  #endif  // #ifdef _MFC_VER

    if ( !bLoaded )
      TRACE(_T("String not loaded 0x%X\n"), ::GetLastError());

    return bLoaded;
  }

#endif  // #ifdef SS_ANSI

  // -------------------------------------------------------------------------
  // FUNCTION:  CStdStr::Format
  //    void _cdecl Formst(CStdStringA& PCSTR szFormat, ...)
  //    void _cdecl Format(PCSTR szFormat);
  //
  // DESCRIPTION:
  //    This function does sprintf/wsprintf style formatting on CStdStringA
  //    objects.  It looks a lot like MFC's CString::Format.  Some people
  //    might even call this identical.  Fortunately, these people are now
  //    dead... heh heh.
  //
  // PARAMETERS:
  //    nId - ID of string resource holding the format string
  //    szFormat - a PCSTR holding the format specifiers
  //    argList - a va_list holding the arguments for the format specifiers.
  //
  // RETURN VALUE:  None.
  // -------------------------------------------------------------------------
  // formatting (using wsprintf style formatting)

    // If they want a Format() function that safely handles string objects
    // without casting

#ifdef SS_SAFE_FORMAT

    // Question:  Joe, you wacky coder you, why do you have so many overloads
    //      of the Format() function
    // Answer:  One reason only - CString compatability.  In short, by making
    //      the Format() function a template this way, I can do strong typing
    //      and allow people to pass CStdString arguments as fillers for
    //      "%s" format specifiers without crashing their program!  The downside
    //      is that I need to overload on the number of arguments.   If you are
    //      passing more arguments than I have listed below in any of my
    //      overloads, just add another one.
    //
    //      Yes, yes, this is really ugly.  In essence what I am doing here is
    //      protecting people from a bad (and incorrect) programming practice
    //      that they should not be doing anyway.  I am protecting them from
    //      themselves.  Why am I doing this?  Well, if you had any idea the
    //      number of times I've been emailed by people about this
    //      "incompatability" in my code, you wouldn't ask.

  void Fmt(const CT* szFmt, ...)
  {
    va_list argList;
    va_start(argList, szFmt);
    FormatV(szFmt, argList);
    va_end(argList);
  }

#ifndef SS_ANSI

    void Format(UINT nId)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
            this->swap(strFmt);
    }
    template<class A1>
    void Format(UINT nId, const A1& v)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
            Fmt(strFmt, FmtArg<A1>(v)());
    }
    template<class A1, class A2>
    void Format(UINT nId, const A1& v1, const A2& v2)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
           Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)());
    }
    template<class A1, class A2, class A3>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)());
        }
    }
    template<class A1, class A2, class A3, class A4>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(),FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(),FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
           Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(), FmtArg<A10>(v10)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(),FmtArg<A10>(v10)(),FmtArg<A11>(v11)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
                FmtArg<A12>(v12)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
                FmtArg<A12>(v12)(), FmtArg<A13>(v13)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
                FmtArg<A12>(v12)(), FmtArg<A13>(v13)(),FmtArg<A14>(v14)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14, class A15>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14, const A15& v15)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
                FmtArg<A12>(v12)(),FmtArg<A13>(v13)(),FmtArg<A14>(v14)(),
                FmtArg<A15>(v15)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14, class A15, class A16>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14, const A15& v15,
                const A16& v16)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
                FmtArg<A12>(v12)(),FmtArg<A13>(v13)(),FmtArg<A14>(v14)(),
                FmtArg<A15>(v15)(), FmtArg<A16>(v16)());
        }
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14, class A15, class A16, class A17>
    void Format(UINT nId, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14, const A15& v15,
                const A16& v16, const A17& v17)
    {
    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
        {
            Fmt(strFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
                FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
                FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
                FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
                FmtArg<A12>(v12)(),FmtArg<A13>(v13)(),FmtArg<A14>(v14)(),
                FmtArg<A15>(v15)(),FmtArg<A16>(v16)(),FmtArg<A17>(v17)());
        }
    }

#endif // #ifndef SS_ANSI

    // ...now the other overload of Format: the one that takes a string literal

    void Format(const CT* szFmt)
    {
        *this = szFmt;
    }
    template<class A1>
    void Format(const CT* szFmt, const A1& v)
    {
        Fmt(szFmt, FmtArg<A1>(v)());
    }
    template<class A1, class A2>
    void Format(const CT* szFmt, const A1& v1, const A2& v2)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)());
    }
    template<class A1, class A2, class A3>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)());
    }
    template<class A1, class A2, class A3, class A4>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)());
    }
    template<class A1, class A2, class A3, class A4, class A5>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(), FmtArg<A10>(v10)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(),FmtArg<A10>(v10)(),FmtArg<A11>(v11)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
            FmtArg<A12>(v12)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
            FmtArg<A12>(v12)(), FmtArg<A13>(v13)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
            FmtArg<A12>(v12)(), FmtArg<A13>(v13)(),FmtArg<A14>(v14)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14, class A15>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14, const A15& v15)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
            FmtArg<A12>(v12)(),FmtArg<A13>(v13)(),FmtArg<A14>(v14)(),
            FmtArg<A15>(v15)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14, class A15, class A16>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14, const A15& v15,
                const A16& v16)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
            FmtArg<A12>(v12)(),FmtArg<A13>(v13)(),FmtArg<A14>(v14)(),
            FmtArg<A15>(v15)(), FmtArg<A16>(v16)());
    }
    template<class A1, class A2, class A3, class A4, class A5, class A6,
        class A7, class A8, class A9, class A10, class A11, class A12,
        class A13, class A14, class A15, class A16, class A17>
    void Format(const CT* szFmt, const A1& v1, const A2& v2, const A3& v3,
                const A4& v4, const A5& v5, const A6& v6, const A7& v7,
                const A8& v8, const A9& v9, const A10& v10, const A11& v11,
                const A12& v12, const A13& v13, const A14& v14, const A15& v15,
                const A16& v16, const A17& v17)
    {
        Fmt(szFmt, FmtArg<A1>(v1)(), FmtArg<A2>(v2)(),
            FmtArg<A3>(v3)(), FmtArg<A4>(v4)(), FmtArg<A5>(v5)(),
            FmtArg<A6>(v6)(), FmtArg<A7>(v7)(), FmtArg<A8>(v8)(),
            FmtArg<A9>(v9)(), FmtArg<A10>(v10)(),FmtArg<A11>(v11)(),
            FmtArg<A12>(v12)(),FmtArg<A13>(v13)(),FmtArg<A14>(v14)(),
            FmtArg<A15>(v15)(),FmtArg<A16>(v16)(),FmtArg<A17>(v17)());
    }

#else  // #ifdef SS_SAFE_FORMAT


#ifndef SS_ANSI

  void Format(UINT nId, ...)
  {
    va_list argList;
    va_start(argList, nId);

    MYTYPE strFmt;
    if ( strFmt.Load(nId) )
      FormatV(strFmt, argList);

    va_end(argList);
  }

#endif  // #ifdef SS_ANSI

  void Format(const CT* szFmt, ...)
  {
    va_list argList;
    va_start(argList, szFmt);
    FormatV(szFmt, argList);
    va_end(argList);
  }

#endif // #ifdef SS_SAFE_FORMAT

  void AppendFormat(const CT* szFmt, ...)
  {
    va_list argList;
    va_start(argList, szFmt);
    AppendFormatV(szFmt, argList);
    va_end(argList);
  }

  #define MAX_FMT_TRIES    5   // #of times we try
  #define FMT_BLOCK_SIZE    2048 // # of bytes to increment per try
  #define BUFSIZE_1ST  256
  #define BUFSIZE_2ND 512
  #define STD_BUF_SIZE    1024

  // an efficient way to add formatted characters to the string.  You may only
  // add up to STD_BUF_SIZE characters at a time, though
  void AppendFormatV(const CT* szFmt, va_list argList)
  {
    CT szBuf[STD_BUF_SIZE];
    int nLen = ssnprintf(szBuf, STD_BUF_SIZE-1, szFmt, argList);

    if ( 0 < nLen )
      this->append(szBuf, nLen);
  }

  // -------------------------------------------------------------------------
  // FUNCTION:  FormatV
  //    void FormatV(PCSTR szFormat, va_list, argList);
  //
  // DESCRIPTION:
  //    This function formats the string with sprintf style format-specs.
  //    It makes a general guess at required buffer size and then tries
  //    successively larger buffers until it finds one big enough or a
  //    threshold (MAX_FMT_TRIES) is exceeded.
  //
  // PARAMETERS:
  //    szFormat - a PCSTR holding the format of the output
  //    argList - a Microsoft specific va_list for variable argument lists
  //
  // RETURN VALUE:
  // -------------------------------------------------------------------------

  // NOTE: Changed by JM to actually function under non-win32,
  //       and to remove the upper limit on size.
  void FormatV(const CT* szFormat, va_list argList)
  {
    // try and grab a sufficient buffersize
    int nChars = FMT_BLOCK_SIZE;
    va_list argCopy;

    CT *p = reinterpret_cast<CT*>(malloc(sizeof(CT)*nChars));
    if (!p) return;

    while (1)
    {
      va_copy(argCopy, argList);

      int nActual = ssvsprintf(p, nChars, szFormat, argCopy);
      /* If that worked, return the string. */
      if (nActual > -1 && nActual < nChars)
      { /* make sure it's NULL terminated */
        p[nActual] = '\0';
        this->assign(p, nActual);
        free(p);
        va_end(argCopy);
        return;
      }
      /* Else try again with more space. */
      if (nActual > -1)        /* glibc 2.1 */
        nChars = nActual + 1;  /* precisely what is needed */
      else                     /* glibc 2.0 */
        nChars *= 2;           /* twice the old size */

      CT *np = reinterpret_cast<CT*>(realloc(p, sizeof(CT)*nChars));
      if (np == NULL)
      {
        free(p);
        va_end(argCopy);
        return;   // failed :(
      }
      p = np;
      va_end(argCopy);
    }
  }

  // -------------------------------------------------------------------------
  // CString Facade Functions:
  //
  // The following methods are intended to allow you to use this class as a
  // near drop-in replacement for CString.
  // -------------------------------------------------------------------------
  #ifdef SS_WIN32
    BSTR AllocSysString() const
    {
      ostring os;
      ssasn(os, *this);
      return ::SysAllocString(os.c_str());
    }
  #endif

#ifndef SS_NO_LOCALE
  int Collate(PCMYSTR szThat) const
  {
    return sscoll(this->c_str(), this->length(), szThat, sslen(szThat));
  }

  int CollateNoCase(PCMYSTR szThat) const
  {
    return ssicoll(this->c_str(), this->length(), szThat, sslen(szThat));
  }
#endif
  int Compare(PCMYSTR szThat) const
  {
    return this->compare(szThat);
  }

  int CompareNoCase(PCMYSTR szThat)  const
  {
    return ssicmp(this->c_str(), szThat);
  }

  int Delete(int nIdx, int nCount=1)
  {
        if ( nIdx < 0 )
      nIdx = 0;

    if ( nIdx < this->GetLength() )
      this->erase(static_cast<MYSIZE>(nIdx), static_cast<MYSIZE>(nCount));

    return GetLength();
  }

  void Empty()
  {
    this->erase();
  }

  int Find(CT ch) const
  {
    MYSIZE nIdx  = this->find_first_of(ch);
    return static_cast<int>(MYBASE::npos == nIdx  ? -1 : nIdx);
  }

  int Find(PCMYSTR szSub) const
  {
    MYSIZE nIdx  = this->find(szSub);
    return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
  }

  int Find(CT ch, int nStart) const
  {
    // CString::Find docs say add 1 to nStart when it's not zero
    // CString::Find code doesn't do that however.  We'll stick
    // with what the code does

    MYSIZE nIdx  = this->find_first_of(ch, static_cast<MYSIZE>(nStart));
    return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
  }

  int Find(PCMYSTR szSub, int nStart) const
  {
    // CString::Find docs say add 1 to nStart when it's not zero
    // CString::Find code doesn't do that however.  We'll stick
    // with what the code does

    MYSIZE nIdx  = this->find(szSub, static_cast<MYSIZE>(nStart));
    return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
  }

  int FindOneOf(PCMYSTR szCharSet) const
  {
    MYSIZE nIdx = this->find_first_of(szCharSet);
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
    VERIFY(sFormat.LoadString(nFormatId));
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

  // GetAllocLength -- an MSVC7 function but it costs us nothing to add it.

  int GetAllocLength()
  {
    return static_cast<int>(this->capacity());
  }

  // -------------------------------------------------------------------------
  // GetXXXX -- Direct access to character buffer
  // -------------------------------------------------------------------------
  CT GetAt(int nIdx) const
  {
    return this->at(static_cast<MYSIZE>(nIdx));
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
    return static_cast<int>(this->length());
  }

  int Insert(int nIdx, CT ch)
  {
    if ( static_cast<MYSIZE>(nIdx) > this->size()-1 )
      this->append(1, ch);
    else
      this->insert(static_cast<MYSIZE>(nIdx), 1, ch);

    return GetLength();
  }
  int Insert(int nIdx, PCMYSTR sz)
  {
    if ( static_cast<MYSIZE>(nIdx) >= this->size() )
      this->append(sz, static_cast<MYSIZE>(sslen(sz)));
    else
      this->insert(static_cast<MYSIZE>(nIdx), sz);

    return GetLength();
  }

  bool IsEmpty() const
  {
    return this->empty();
  }

  MYTYPE Left(int nCount) const
  {
        // Range check the count.

    nCount = SSMAX(0, SSMIN(nCount, static_cast<int>(this->size())));
    return this->substr(0, static_cast<MYSIZE>(nCount));
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
    std::reverse(this->begin(), this->end());
  }

  void MakeUpper()
  {
    ToUpper();
  }

  MYTYPE Mid(int nFirst) const
  {
    return Mid(nFirst, this->GetLength()-nFirst);
  }

  MYTYPE Mid(int nFirst, int nCount) const
  {
    // CString does range checking here.  Since we're trying to emulate it,
    // we must check too.

    if ( nFirst < 0 )
      nFirst = 0;
    if ( nCount < 0 )
      nCount = 0;

    int nSize = static_cast<int>(this->size());

    if ( nFirst + nCount > nSize )
      nCount = nSize - nFirst;

    if ( nFirst > nSize )
      return MYTYPE();

    ASSERT(nFirst >= 0);
    ASSERT(nFirst + nCount <= nSize);

    return this->substr(static_cast<MYSIZE>(nFirst),
              static_cast<MYSIZE>(nCount));
  }

  void ReleaseBuffer(int nNewLen=-1)
  {
    RelBuf(nNewLen);
  }

  int Remove(CT ch)
  {
    MYSIZE nIdx    = 0;
    int nRemoved  = 0;
    while ( (nIdx=this->find_first_of(ch)) != MYBASE::npos )
    {
      this->erase(nIdx, 1);
      nRemoved++;
    }
    return nRemoved;
  }

  int Replace(CT chOld, CT chNew)
  {
    int nReplaced  = 0;

    for ( MYITER iter=this->begin(); iter != this->end(); iter++ )
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
    int nReplaced    = 0;
    MYSIZE nIdx      = 0;
    MYSIZE nOldLen    = sslen(szOld);

    if ( 0 != nOldLen )
    {
      // If the replacement string is longer than the one it replaces, this
      // string is going to have to grow in size,  Figure out how much
      // and grow it all the way now, rather than incrementally

      MYSIZE nNewLen    = sslen(szNew);
      if ( nNewLen > nOldLen )
      {
        int nFound      = 0;
        while ( nIdx < this->length() &&
          (nIdx=this->find(szOld, nIdx)) != MYBASE::npos )
        {
          nFound++;
          nIdx += nOldLen;
        }
        this->reserve(this->size() + nFound * (nNewLen - nOldLen));
      }


      static const CT ch  = CT(0);
      PCMYSTR szRealNew  = szNew == 0 ? &ch : szNew;
      nIdx        = 0;

      while ( nIdx < this->length() &&
        (nIdx=this->find(szOld, nIdx)) != MYBASE::npos )
      {
        this->replace(this->begin()+nIdx, this->begin()+nIdx+nOldLen,
          szRealNew);

        nReplaced++;
        nIdx += nNewLen;
      }
    }

    return nReplaced;
  }

  int ReverseFind(CT ch) const
  {
    MYSIZE nIdx  = this->find_last_of(ch);
    return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
  }

  // ReverseFind overload that's not in CString but might be useful
  int ReverseFind(PCMYSTR szFind, MYSIZE pos=MYBASE::npos) const
  {
    //yuvalt - this does not compile with g++ since MYTTYPE() is different type
    //MYSIZE nIdx  = this->rfind(0 == szFind ? MYTYPE() : szFind, pos);
    MYSIZE nIdx  = this->rfind(0 == szFind ? "" : szFind, pos);
    return static_cast<int>(MYBASE::npos == nIdx ? -1 : nIdx);
  }

  MYTYPE Right(int nCount) const
  {
        // Range check the count.

    nCount = SSMAX(0, SSMIN(nCount, static_cast<int>(this->size())));
    return this->substr(this->size()-static_cast<MYSIZE>(nCount));
  }

  void SetAt(int nIndex, CT ch)
  {
    ASSERT(this->size() > static_cast<MYSIZE>(nIndex));
    this->at(static_cast<MYSIZE>(nIndex))    = ch;
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
        MYSIZE pos = this->find_first_of(szCharSet);
        return pos == MYBASE::npos ? *this : Left(pos);
  }

  MYTYPE SpanIncluding(PCMYSTR szCharSet) const
  {
        MYSIZE pos = this->find_first_not_of(szCharSet);
        return pos == MYBASE::npos ? *this : Left(pos);
  }

#if defined SS_WIN32 && !defined(UNICODE) && !defined(SS_ANSI)

  // CString's OemToAnsi and AnsiToOem functions are available only in
  // Unicode builds.  However since we're a template we also need a
  // runtime check of CT and a reinterpret_cast to account for the fact
  // that CStdStringW gets instantiated even in non-Unicode builds.

  void AnsiToOem()
  {
    if ( sizeof(CT) == sizeof(char) && !empty() )
    {
      ::CharToOem(reinterpret_cast<PCSTR>(this->c_str()),
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
      ::OemToChar(reinterpret_cast<PCSTR>(this->c_str()),
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
    this->erase(this->begin(),
      std::find_if(this->begin(), this->end(), NotSpace<CT>()));

    return *this;
  }

  MYTYPE&  TrimLeft(CT tTrim)
  {
    this->erase(0, this->find_first_not_of(tTrim));
    return *this;
  }

  MYTYPE&  TrimLeft(PCMYSTR szTrimChars)
  {
    this->erase(0, this->find_first_not_of(szTrimChars));
    return *this;
  }

  MYTYPE& TrimRight()
  {
    // NOTE:  When comparing reverse_iterators here (MYRITER), I avoid using
    // operator!=.  This is because namespace rel_ops also has a template
    // operator!= which conflicts with the global operator!= already defined
    // for reverse_iterator in the header <utility>.
    // Thanks to John James for alerting me to this.

    MYRITER it = std::find_if(this->rbegin(), this->rend(), NotSpace<CT>());
    if ( !(this->rend() == it) )
      this->erase(this->rend() - it);

    this->erase(!(it == this->rend()) ? this->find_last_of(*it) + 1 : 0);
    return *this;
  }

  MYTYPE&  TrimRight(CT tTrim)
  {
    MYSIZE nIdx  = this->find_last_not_of(tTrim);
    this->erase(MYBASE::npos == nIdx ? 0 : ++nIdx);
    return *this;
  }

  MYTYPE&  TrimRight(PCMYSTR szTrimChars)
  {
    MYSIZE nIdx  = this->find_last_not_of(szTrimChars);
    this->erase(MYBASE::npos == nIdx ? 0 : ++nIdx);
    return *this;
  }

  void      FreeExtra()
  {
    MYTYPE mt;
    this->swap(mt);
    if ( !mt.empty() )
      this->assign(mt.c_str(), mt.size());
  }

  // I have intentionally not implemented the following CString
  // functions.   You cannot make them work without taking advantage
  // of implementation specific behavior.  However if you absolutely
  // MUST have them, uncomment out these lines for "sort-of-like"
  // their behavior.  You're on your own.

//  CT*        LockBuffer()  { return GetBuf(); }// won't really lock
//  void      UnlockBuffer(); { }  // why have UnlockBuffer w/o LockBuffer?

  // Array-indexing operators.  Required because we defined an implicit cast
  // to operator const CT* (Thanks to Julian Selman for pointing this out)

  CT& operator[](int nIdx)
  {
    return static_cast<MYBASE*>(this)->operator[](static_cast<MYSIZE>(nIdx));
  }

  const CT& operator[](int nIdx) const
  {
    return static_cast<const MYBASE*>(this)->operator[](static_cast<MYSIZE>(nIdx));
  }

  CT& operator[](unsigned int nIdx)
  {
    return static_cast<MYBASE*>(this)->operator[](static_cast<MYSIZE>(nIdx));
  }

  const CT& operator[](unsigned int nIdx) const
  {
    return static_cast<const MYBASE*>(this)->operator[](static_cast<MYSIZE>(nIdx));
  }

  CT& operator[](unsigned long nIdx)
  {
    return static_cast<MYBASE*>(this)->operator[](static_cast<MYSIZE>(nIdx));
  }

  const CT& operator[](unsigned long nIdx) const
  {
    return static_cast<const MYBASE*>(this)->operator[](static_cast<MYSIZE>(nIdx));
  }

#ifndef SS_NO_IMPLICIT_CAST
  operator const CT*() const
  {
    return this->c_str();
  }
#endif

  // IStream related functions.  Useful in IPersistStream implementations

#ifdef SS_INC_COMDEF

  // struct SSSHDR - useful for non Std C++ persistence schemes.
  typedef struct SSSHDR
  {
    BYTE  byCtrl;
    ULONG  nChars;
  } SSSHDR;  // as in "Standard String Stream Header"

  #define SSSO_UNICODE  0x01  // the string is a wide string
  #define SSSO_COMPRESS  0x02  // the string is compressed

  // -------------------------------------------------------------------------
  // FUNCTION: StreamSize
  // REMARKS:
  //    Returns how many bytes it will take to StreamSave() this CStdString
  //    object to an IStream.
  // -------------------------------------------------------------------------
  ULONG StreamSize() const
  {
    // Control header plus string
    ASSERT(this->size()*sizeof(CT) < 0xffffffffUL - sizeof(SSSHDR));
    return (this->size() * sizeof(CT)) + sizeof(SSSHDR);
  }

  // -------------------------------------------------------------------------
  // FUNCTION: StreamSave
  // REMARKS:
  //    Saves this CStdString object to a COM IStream.
  // -------------------------------------------------------------------------
  HRESULT StreamSave(IStream* pStream) const
  {
    ASSERT(this->size()*sizeof(CT) < 0xffffffffUL - sizeof(SSSHDR));
    HRESULT hr    = E_FAIL;
    ASSERT(pStream != 0);
    SSSHDR hdr;
    hdr.byCtrl    = sizeof(CT) == 2 ? SSSO_UNICODE : 0;
    hdr.nChars    = this->size();


    if ( FAILED(hr=pStream->Write(&hdr, sizeof(SSSHDR), 0)) )
    {
      TRACE(_T("StreamSave: Cannot write control header, ERR=0x%X\n"),hr);
    }
    else if ( empty() )
    {
      ;    // nothing to write
    }
    else if ( FAILED(hr=pStream->Write(this->c_str(),
      this->size()*sizeof(CT), 0)) )
    {
      TRACE(_T("StreamSave: Cannot write string to stream 0x%X\n"), hr);
    }

    return hr;
  }


  // -------------------------------------------------------------------------
  // FUNCTION: StreamLoad
  // REMARKS:
  //    This method loads the object from an IStream.
  // -------------------------------------------------------------------------
  HRESULT StreamLoad(IStream* pStream)
  {
    ASSERT(pStream != 0);
    SSSHDR hdr;
    HRESULT hr      = E_FAIL;

    if ( FAILED(hr=pStream->Read(&hdr, sizeof(SSSHDR), 0)) )
    {
      TRACE(_T("StreamLoad: Cant read control header, ERR=0x%X\n"), hr);
    }
    else if ( hdr.nChars > 0 )
    {
      ULONG nRead    = 0;
      PMYSTR pMyBuf  = BufferSet(hdr.nChars);

      // If our character size matches the character size of the string
      // we're trying to read, then we can read it directly into our
      // buffer. Otherwise, we have to read into an intermediate buffer
      // and convert.

      if ( (hdr.byCtrl & SSSO_UNICODE) != 0 )
      {
        ULONG nBytes  = hdr.nChars * sizeof(wchar_t);
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
        ULONG nBytes  = hdr.nChars * sizeof(char);
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
// MSVC USERS: HOW TO EXPORT CSTDSTRING FROM A DLL
//
// If you are using MS Visual C++ and you want to export CStdStringA and
// CStdStringW from a DLL, then all you need to
//
//    1.  make sure that all components link to the same DLL version
//      of the CRT (not the static one).
//    2.  Uncomment the 3 lines of code below
//    3.  #define 2 macros per the instructions in MS KnowledgeBase
//      article Q168958.  The macros are:
//
//    MACRO    DEFINTION WHEN EXPORTING    DEFINITION WHEN IMPORTING
//    -----    ------------------------    -------------------------
//    SSDLLEXP  (nothing, just #define it)    extern
//    SSDLLSPEC  __declspec(dllexport)      __declspec(dllimport)
//
//    Note that these macros must be available to ALL clients who want to
//    link to the DLL and use the class.  If they
//
// A word of advice: Don't bother.
//
// Really, it is not necessary to export CStdString functions from a DLL.  I
// never do.  In my projects, I do generally link to the DLL version of the
// Standard C++ Library, but I do NOT attempt to export CStdString functions.
// I simply include the header where it is needed and allow for the code
// redundancy.
//
// That redundancy is a lot less than you think.  This class does most of its
// work via the Standard C++ Library, particularly the base_class basic_string<>
// member functions.  Most of the functions here are small enough to be inlined
// anyway.  Besides, you'll find that in actual practice you use less than 1/2
// of the code here, even in big projects and different modules will use as
// little as 10% of it.  That means a lot less functions actually get linked
// your binaries.  If you export this code from a DLL, it ALL gets linked in.
//
// I've compared the size of the binaries from exporting vs NOT exporting.  Take
// my word for it -- exporting this code is not worth the hassle.
//
// -----------------------------------------------------------------------------
//#pragma warning(disable:4231) // non-standard extension ("extern template")
//  SSDLLEXP template class SSDLLSPEC CStdStr<char>;
//  SSDLLEXP template class SSDLLSPEC CStdStr<wchar_t>;


// =============================================================================
//            END OF CStdStr INLINE FUNCTION DEFINITIONS
// =============================================================================

//  Now typedef our class names based upon this humongous template

typedef CStdStr<char>    CStdStringA;  // a better std::string
typedef CStdStr<wchar_t>  CStdStringW;  // a better std::wstring
typedef CStdStr<uint16_t>  CStdString16;  // a 16bit char string
typedef CStdStr<uint32_t>  CStdString32;  // a 32bit char string
typedef CStdStr<OLECHAR>  CStdStringO;  // almost always CStdStringW

// -----------------------------------------------------------------------------
// CStdStr addition functions defined as inline
// -----------------------------------------------------------------------------


inline CStdStringA operator+(const CStdStringA& s1, const CStdStringA& s2)
{
  CStdStringA sRet(SSREF(s1));
  sRet.append(s2);
  return sRet;
}
inline CStdStringA operator+(const CStdStringA& s1, CStdStringA::value_type t)
{
  CStdStringA sRet(SSREF(s1));
  sRet.append(1, t);
  return sRet;
}
inline CStdStringA operator+(const CStdStringA& s1, PCSTR pA)
{
  CStdStringA sRet(SSREF(s1));
  sRet.append(pA);
  return sRet;
}
inline CStdStringA operator+(PCSTR pA, const CStdStringA& sA)
{
  CStdStringA sRet;
  CStdStringA::size_type nObjSize = sA.size();
  CStdStringA::size_type nLitSize =
    static_cast<CStdStringA::size_type>(sslen(pA));

  sRet.reserve(nLitSize + nObjSize);
  sRet.assign(pA);
  sRet.append(sA);
  return sRet;
}


inline CStdStringA operator+(const CStdStringA& s1, const CStdStringW& s2)
{
  return s1 + CStdStringA(s2);
}
inline CStdStringW operator+(const CStdStringW& s1, const CStdStringW& s2)
{
  CStdStringW sRet(SSREF(s1));
  sRet.append(s2);
  return sRet;
}
inline CStdStringA operator+(const CStdStringA& s1, PCWSTR pW)
{
  return s1 + CStdStringA(pW);
}

#ifdef UNICODE
  inline CStdStringW operator+(PCWSTR pW, const CStdStringA& sA)
  {
    return CStdStringW(pW) + CStdStringW(SSREF(sA));
  }
  inline CStdStringW operator+(PCSTR pA, const CStdStringW& sW)
  {
    return CStdStringW(pA) + sW;
  }
#else
  inline CStdStringA operator+(PCWSTR pW, const CStdStringA& sA)
  {
    return CStdStringA(pW) + sA;
  }
  inline CStdStringA operator+(PCSTR pA, const CStdStringW& sW)
  {
    return pA + CStdStringA(sW);
  }
#endif

// ...Now the wide string versions.
inline CStdStringW operator+(const CStdStringW& s1, CStdStringW::value_type t)
{
  CStdStringW sRet(SSREF(s1));
  sRet.append(1, t);
  return sRet;
}
inline CStdStringW operator+(const CStdStringW& s1, PCWSTR pW)
{
  CStdStringW sRet(SSREF(s1));
  sRet.append(pW);
  return sRet;
}
inline CStdStringW operator+(PCWSTR pW, const CStdStringW& sW)
{
  CStdStringW sRet;
  CStdStringW::size_type nObjSize = sW.size();
  CStdStringA::size_type nLitSize =
    static_cast<CStdStringW::size_type>(sslen(pW));

  sRet.reserve(nLitSize + nObjSize);
  sRet.assign(pW);
  sRet.append(sW);
  return sRet;
}

inline CStdStringW operator+(const CStdStringW& s1, const CStdStringA& s2)
{
  return s1 + CStdStringW(s2);
}
inline CStdStringW operator+(const CStdStringW& s1, PCSTR pA)
{
  return s1 + CStdStringW(pA);
}


// New-style format function is a template

#ifdef SS_SAFE_FORMAT

template<>
struct FmtArg<CStdStringA>
{
    explicit FmtArg(const CStdStringA& arg) : a_(arg) {}
    PCSTR operator()() const { return a_.c_str(); }
    const CStdStringA& a_;
private:
    FmtArg<CStdStringA>& operator=(const FmtArg<CStdStringA>&) { return *this; }
};
template<>
struct FmtArg<CStdStringW>
{
    explicit FmtArg(const CStdStringW& arg) : a_(arg) {}
    PCWSTR operator()() const { return a_.c_str(); }
    const CStdStringW& a_;
private:
    FmtArg<CStdStringW>& operator=(const FmtArg<CStdStringW>&) { return *this; }
};

template<>
struct FmtArg<std::string>
{
    explicit FmtArg(const std::string& arg) : a_(arg) {}
    PCSTR operator()() const { return a_.c_str(); }
    const std::string& a_;
private:
    FmtArg<std::string>& operator=(const FmtArg<std::string>&) { return *this; }
};
template<>
struct FmtArg<std::wstring>
{
    explicit FmtArg(const std::wstring& arg) : a_(arg) {}
    PCWSTR operator()() const { return a_.c_str(); }
    const std::wstring& a_;
private:
    FmtArg<std::wstring>& operator=(const FmtArg<std::wstring>&) {return *this;}
};
#endif // #ifdef SS_SAFEFORMAT

#ifndef SS_ANSI
  // SSResourceHandle: our MFC-like resource handle
  inline HMODULE& SSResourceHandle()
  {
    static HMODULE hModuleSS  = GetModuleHandle(0);
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
    CString strTemp  = strA;
    return ar << strTemp;
  }
  inline CArchive& AFXAPI operator<<(CArchive& ar, const CStdStringW& strW)
  {
    CString strTemp  = strW;
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
#endif  // #ifdef _MFC_VER -- (i.e. is this MFC?)



// -----------------------------------------------------------------------------
// GLOBAL FUNCTION:  WUFormat
//    CStdStringA WUFormat(UINT nId, ...);
//    CStdStringA WUFormat(PCSTR szFormat, ...);
//
// REMARKS:
//    This function allows the caller for format and return a CStdStringA
//    object with a single line of code.
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



#if defined(SS_WIN32) && !defined (SS_ANSI)
  // -------------------------------------------------------------------------
  // FUNCTION: WUSysMessage
  //   CStdStringA WUSysMessageA(DWORD dwError, DWORD dwLangId=SS_DEFLANGID);
  //   CStdStringW WUSysMessageW(DWORD dwError, DWORD dwLangId=SS_DEFLANGID);
  //
  // DESCRIPTION:
  //   This function simplifies the process of obtaining a string equivalent
  //   of a system error code returned from GetLastError().  You simply
  //   supply the value returned by GetLastError() to this function and the
  //   corresponding system string is returned in the form of a CStdStringA.
  //
  // PARAMETERS:
  //   dwError - a DWORD value representing the error code to be translated
  //   dwLangId - the language id to use.  defaults to english.
  //
  // RETURN VALUE:
  //   a CStdStringA equivalent of the error code.  Currently, this function
  //   only returns either English of the system default language strings.
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
  //#define CStdString        CStdStringW
  typedef CStdStringW        CStdString;
  #define WUSysMessage      WUSysMessageW
  #define WUFormat        WUFormatW
#else
  //#define CStdString        CStdStringA
  typedef CStdStringA        CStdString;
  #define WUSysMessage      WUSysMessageA
  #define WUFormat        WUFormatA
#endif

// ...and some shorter names for the space-efficient

#define WUSysMsg          WUSysMessage
#define WUSysMsgA          WUSysMessageA
#define WUSysMsgW          WUSysMessageW
#define WUFmtA            WUFormatA
#define  WUFmtW            WUFormatW
#define WUFmt            WUFormat
#define WULastErrMsg()        WUSysMessage(::GetLastError())
#define WULastErrMsgA()        WUSysMessageA(::GetLastError())
#define WULastErrMsgW()        WUSysMessageW(::GetLastError())


// -----------------------------------------------------------------------------
// FUNCTIONAL COMPARATORS:
// REMARKS:
//    These structs are derived from the std::binary_function template.  They
//    give us functional classes (which may be used in Standard C++ Library
//    collections and algorithms) that perform case-insensitive comparisons of
//    CStdString objects.  This is useful for maps in which the key may be the
//     proper string but in the wrong case.
// -----------------------------------------------------------------------------
#define StdStringLessNoCaseW    SSLNCW  // avoid VC compiler warning 4786
#define StdStringEqualsNoCaseW    SSENCW
#define StdStringLessNoCaseA    SSLNCA
#define StdStringEqualsNoCaseA    SSENCA

#ifdef UNICODE
  #define StdStringLessNoCase    SSLNCW
  #define StdStringEqualsNoCase  SSENCW
#else
  #define StdStringLessNoCase    SSLNCA
  #define StdStringEqualsNoCase  SSENCA
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


// These std::swap specializations come courtesy of Mike Crusader.

//namespace std
//{
//  inline void swap(CStdStringA& s1, CStdStringA& s2) throw()
//  {
//    s1.swap(s2);
//  }
//  template<>
//  inline void swap(CStdStringW& s1, CStdStringW& s2) throw()
//  {
//    s1.swap(s2);
//  }
//}

// Turn back on any Borland warnings we turned off.

#ifdef __BORLANDC__
    #pragma option pop  // Turn back on inline function warnings
//  #pragma warn +inl   // Turn back on inline function warnings
#endif

typedef std::vector<CStdString> CStdStringArray;

#endif  // #ifndef STDSTRING_H

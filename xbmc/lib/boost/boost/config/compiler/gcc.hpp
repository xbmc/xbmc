//  (C) Copyright John Maddock 2001 - 2003. 
//  (C) Copyright Darin Adler 2001 - 2002. 
//  (C) Copyright Jens Maurer 2001 - 2002. 
//  (C) Copyright Beman Dawes 2001 - 2003. 
//  (C) Copyright Douglas Gregor 2002. 
//  (C) Copyright David Abrahams 2002 - 2003. 
//  (C) Copyright Synge Todo 2003. 
//  Use, modification and distribution are subject to the 
//  Boost Software License, Version 1.0. (See accompanying file 
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for most recent version.

//  GNU C++ compiler setup:

#if __GNUC__ < 3
#   if __GNUC_MINOR__ == 91
       // egcs 1.1 won't parse shared_ptr.hpp without this:
#      define BOOST_NO_AUTO_PTR
#   endif
#   if __GNUC_MINOR__ < 95
      //
      // Prior to gcc 2.95 member templates only partly
      // work - define BOOST_MSVC6_MEMBER_TEMPLATES
      // instead since inline member templates mostly work.
      //
#     define BOOST_NO_MEMBER_TEMPLATES
#     if __GNUC_MINOR__ >= 9
#       define BOOST_MSVC6_MEMBER_TEMPLATES
#     endif
#   endif

#   if __GNUC_MINOR__ < 96
#     define BOOST_NO_SFINAE
#   endif

#   if __GNUC_MINOR__ <= 97
#     define BOOST_NO_MEMBER_TEMPLATE_FRIENDS
#     define BOOST_NO_OPERATORS_IN_NAMESPACE
#   endif

#   define BOOST_NO_USING_DECLARATION_OVERLOADS_FROM_TYPENAME_BASE
#   define BOOST_FUNCTION_SCOPE_USING_DECLARATION_BREAKS_ADL
#   define BOOST_NO_IS_ABSTRACT
#elif __GNUC__ == 3
#  if defined (__PATHSCALE__)
#     define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#     define BOOST_NO_IS_ABSTRACT
#  endif
   //
   // gcc-3.x problems:
   //
   // Bug specific to gcc 3.1 and 3.2:
   //
#  if ((__GNUC_MINOR__ == 1) || (__GNUC_MINOR__ == 2))
#     define BOOST_NO_EXPLICIT_FUNCTION_TEMPLATE_ARGUMENTS
#  endif
#  if __GNUC_MINOR__ < 4
#     define BOOST_NO_IS_ABSTRACT
#  endif
#endif
#if __GNUC__ < 4
//
// All problems to gcc-3.x and earlier here:
//
#define BOOST_NO_TWO_PHASE_NAME_LOOKUP
#  ifdef __OPEN64__
#     define BOOST_NO_IS_ABSTRACT
#  endif
#endif

#ifndef __EXCEPTIONS
# define BOOST_NO_EXCEPTIONS
#endif


//
// Threading support: Turn this on unconditionally here (except for
// those platforms where we can know for sure). It will get turned off again
// later if no threading API is detected.
//
#if !defined(__MINGW32__) && !defined(linux) && !defined(__linux) && !defined(__linux__)
# define BOOST_HAS_THREADS
#endif 

//
// gcc has "long long"
//
#define BOOST_HAS_LONG_LONG

//
// gcc implements the named return value optimization since version 3.1
//
#if __GNUC__ > 3 || ( __GNUC__ == 3 && __GNUC_MINOR__ >= 1 )
#define BOOST_HAS_NRVO
#endif
//
// RTTI and typeinfo detection is possible post gcc-4.3:
//
#if __GNUC__ * 100 + __GNUC_MINOR__ >= 403
#  ifndef __GXX_RTTI
#     define BOOST_NO_TYPEID
#     define BOOST_NO_RTTI
#  endif
#endif

// C++0x features not implemented in any GCC version
//
#define BOOST_NO_CONSTEXPR
#define BOOST_NO_EXTERN_TEMPLATE
#define BOOST_NO_LAMBDAS
#define BOOST_NO_NULLPTR
#define BOOST_NO_RAW_LITERALS
#define BOOST_NO_TEMPLATE_ALIASES
#define BOOST_NO_UNICODE_LITERALS

// C++0x features in 4.3.n and later
//
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 2)) && defined(__GXX_EXPERIMENTAL_CXX0X__)
// C++0x features are only enabled when -std=c++0x or -std=gnu++0x are
// passed on the command line, which in turn defines
// __GXX_EXPERIMENTAL_CXX0X__.
#  define BOOST_HAS_DECLTYPE
#  define BOOST_HAS_RVALUE_REFS
#  define BOOST_HAS_STATIC_ASSERT
#  define BOOST_HAS_VARIADIC_TMPL
#else
#  define BOOST_NO_DECLTYPE
#  define BOOST_NO_FUNCTION_TEMPLATE_DEFAULT_ARGS
#  define BOOST_NO_RVALUE_REFERENCES
#  define BOOST_NO_STATIC_ASSERT

// Variadic templates compiler: 
//   http://www.generic-programming.org/~dgregor/cpp/variadic-templates.html
#  ifdef __VARIADIC_TEMPLATES
#    define BOOST_HAS_VARIADIC_TMPL
#  else
#    define BOOST_NO_VARIADIC_TEMPLATES
#  endif
#endif

// C++0x features in 4.4.n and later
//
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4) || !defined(__GXX_EXPERIMENTAL_CXX0X__)
#  define BOOST_NO_AUTO_DECLARATIONS
#  define BOOST_NO_AUTO_MULTIDECLARATIONS
#  define BOOST_NO_CHAR16_T
#  define BOOST_NO_CHAR32_T
#  define BOOST_NO_DEFAULTED_FUNCTIONS
#  define BOOST_NO_DELETED_FUNCTIONS
#  define BOOST_NO_INITIALIZER_LISTS
#  define BOOST_NO_SCOPED_ENUMS  
#endif

#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 4)
#  define BOOST_NO_SFINAE_EXPR
#endif

// C++0x features in 4.4.1 and later
//
#if (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__ < 40401) || !defined(__GXX_EXPERIMENTAL_CXX0X__)
// scoped enums have a serious bug in 4.4.0, so define BOOST_NO_SCOPED_ENUMS before 4.4.1
// See http://gcc.gnu.org/bugzilla/show_bug.cgi?id=38064
#  define BOOST_NO_SCOPED_ENUMS
#endif

// C++0x features in 4.5.n and later
//
#if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5) || !defined(__GXX_EXPERIMENTAL_CXX0X__)
#  define BOOST_NO_EXPLICIT_CONVERSION_OPERATORS
#endif

// ConceptGCC compiler:
//   http://www.generic-programming.org/software/ConceptGCC/
#ifdef __GXX_CONCEPTS__
#  define BOOST_HAS_CONCEPTS
#  define BOOST_COMPILER "ConceptGCC version " __VERSION__
#else
#  define BOOST_NO_CONCEPTS
#endif

#ifndef BOOST_COMPILER
#  define BOOST_COMPILER "GNU C++ version " __VERSION__
#endif

//
// versions check:
// we don't know gcc prior to version 2.90:
#if (__GNUC__ == 2) && (__GNUC_MINOR__ < 90)
#  error "Compiler not configured - please reconfigure"
#endif
//
// last known and checked version is 4.4 (Pre-release):
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 4))
#  if defined(BOOST_ASSERT_CONFIG)
#     error "Unknown compiler version - please run the configure tests and report the results"
#  else
// we don't emit warnings here anymore since there are no defect macros defined for
// gcc post 3.4, so any failures are gcc regressions...
//#     warning "Unknown compiler version - please run the configure tests and report the results"
#  endif
#endif



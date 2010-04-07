/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2007 Johannes Lehtinen
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

/** @file
 * Common defines shared by C-Pluff C and C++ APIs.
 * This file is automatically included by the top level C and C++
 * API header files. There should be no need to include it explicitly.
 */

#ifndef CPLUFFDEF_H_
#define CPLUFFDEF_H_


/* ------------------------------------------------------------------------
 * Version information
 * ----------------------------------------------------------------------*/

/**
 * @defgroup versionInfo Version information
 * @ingroup cDefines cxxDefines
 *
 * C-Pluff version information. Notice that this version information
 * is static version information included in header files. The
 * macros introduced here can be used for compile time checks.
 */
/*@{*/

/**
 * The C-Pluff release version string. This string identifies a specific
 * version of the C-Pluff distribution. Compile time software compatibility
 * checks should use #CP_VERSION_MAJOR and #CP_VERSION_MINOR instead.
 */
#define CP_VERSION "0.1.3"

/**
 * The major version number component of the release version. This is an
 * integer.
 */
#define CP_VERSION_MAJOR 0

/**
 * The minor version number component of the release version. This is an
 * integer.
 */
#define CP_VERSION_MINOR 1

/*@}*/


/* ------------------------------------------------------------------------
 * Symbol visibility
 * ----------------------------------------------------------------------*/

/**
 * @defgroup symbolVisibility Symbol visibility
 * @ingroup cDefines cxxDefines
 *
 * Macros for controlling inter-module symbol visibility and linkage. These
 * macros have platform specific values. #CP_EXPORT, #CP_IMPORT and #CP_HIDDEN
 * can be reused by plug-in implementations for better portability. The
 * complexity is mostly due to Windows DLL exports and imports.
 *
 * @anchor symbolVisibilityExample
 * Each module should usually define its own macro to declare API symbols with
 * #CP_EXPORT and #CP_IMPORT as necessary. For example, a mobule could define
 * a macro @c MY_API in the API header file as follows.
 *
 * @code
 * #ifndef MY_API
 * #  define MY_API CP_IMPORT
 * #endif
 * @endcode
 *
 * By default the API symbols would then be marked for import which is correct
 * when client modules are including the API header file. When compiling the
 * module itself the option @c -DMY_API=CP_EXPORT would be passed to the compiler to
 * override the API header file and to mark the API symbols for export.
 * The overriding definition could also be included in module source files or
 * in an internal header file before including the API header file.
 */
/*@{*/

/**
 * @def CP_EXPORT
 *
 * Declares a symbol to be exported for inter-module usage. When compiling the
 * module which defines the symbol this macro should be placed
 * at the start of the symbol declaration to ensure that the symbol is exported
 * to other modules. However, when compiling other modules the declaration of
 * the symbol should start with #CP_IMPORT.
 * See @ref symbolVisibilityExample "the example" of how to do this.
 */

/**
 * @def CP_IMPORT
 *
 * Declares a symbol to be imported from another module. When compiling a
 * module which uses the symbol this macro should be placed at the start of
 * the symbol declaration to ensure that the symbol is imported from the
 * defining module. However, when compiling the defining module the declaration
 * of the symbol should start with #CP_EXPORT.
 * See @ref symbolVisibilityExample "the example" of how to do this.
 */

/**
 * @def CP_HIDDEN
 *
 * Declares a symbol hidden from other modules. This macro should be
 * placed at the start of the symbol declaration to hide the symbol from other
 * modules (if supported by the platform). This macro is not intended to be
 * used with symbols declared as "static" which are already internal to the
 * object file. Some platforms do not support hiding of symbols and therefore
 * unique prefixes should be used for global symbols internal to the module
 * even when they are declared using this macro.
 */

#if defined(_WIN32)
#  define CP_EXPORT __declspec(dllexport)
#  define CP_IMPORT extern __declspec(dllimport)
#  define CP_HIDDEN
#elif defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#  define CP_EXPORT
#  define CP_IMPORT extern
#  define CP_HIDDEN __attribute__ ((visibility ("hidden")))
#else
#  define CP_EXPORT
#  define CP_IMPORT extern
#  define CP_HIDDEN
#endif

/*@}*/


/* ------------------------------------------------------------------------
 * GCC attributes
 * ----------------------------------------------------------------------*/

/**
 * @defgroup cDefinesGCCAttributes GCC attributes
 * @ingroup cDefines cxxDefines
 *
 * These macros conditionally define GCC attributes for declarations.
 * They are used in C-Pluff API declarations to enable better optimization
 * and error checking when using GCC. In non-GCC platforms they have
 * empty values.
 */
/*@{*/

/**
 * @def CP_GCC_PURE
 *
 * Declares a function as pure function having no side effects.
 * This attribute is supported in GCC since version 2.96.
 * Such functions can be subject to common subexpression elimination
 * and loop optimization.
 */

/**
 * @def CP_GCC_NONNULL
 *
 * Specifies that some pointer arguments to a function should have
 * non-NULL values. Takes a variable length list of argument indexes as
 * arguments. This attribute is supported in GCC since version 3.3.
 * It can be used for enhanced error checking and some optimizations.
 */

#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#define CP_GCC_PURE __attribute__((pure))
#else
#define CP_GCC_PURE
#endif
#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#define CP_GCC_NONNULL(...) __attribute__((nonnull (__VA_ARGS__)))
#else
#define CP_GCC_NONNULL(...)
#endif

/*@}*/

#endif /*CPLUFFDEF_H_*/

/* runtime.hpp                                
 *
 * Copyright (C) 2003 Sawtooth Consulting Ltd.
 *
 * This file is part of yaSSL, an SSL implementation written by Todd A Ouska
 * (todd at yassl.com, see www.yassl.com).
 *
 * yaSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * There are special exceptions to the terms and conditions of the GPL as it
 * is applied to yaSSL. View the full text of the exception in the file
 * FLOSS-EXCEPTIONS in the directory of this software distribution.
 *
 * yaSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

/* runtime.hpp provides C++ runtime support functions when building a pure C
 * version of yaSSL, user must define YASSL_PURE_C
*/



#ifndef yaSSL_NEW_HPP
#define yaSSL_NEW_HPP

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef __sun
 
#include <assert.h>

// Handler for pure virtual functions
namespace __Crun {
    static void pure_error(void)
    {
       assert(0); // "Pure virtual method called, Aborted", GCC 4.2 str cmp fix
    }
} // namespace __Crun

#endif // __sun


#if defined(__GNUC__) && !(defined(__ICC) || defined(__INTEL_COMPILER))

#if __GNUC__ > 2

extern "C" {
#if !defined(DO_TAOCRYPT_KERNEL_MODE)
    #include <assert.h>
#else
    #include "kernelc.hpp"
#endif

/* Disallow inline __cxa_pure_virtual() */
static int __cxa_pure_virtual() __attribute__((noinline, used));
static int __cxa_pure_virtual()
{
    // oops, pure virtual called!
    assert("Pure virtual method called." == "Aborted");
    return 0;
}

} // extern "C"

#endif // __GNUC__ > 2
#endif // compiler check
#endif // yaSSL_NEW_HPP


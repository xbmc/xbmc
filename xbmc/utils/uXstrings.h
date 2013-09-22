/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

/** @file utils/uXstrings.h
 *  Declarations of std::u16string and std::u32string for systems without those declarations
 */
#pragma once

#include <string>

#ifndef TARGET_WINDOWS
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#if !defined(HAVE_STD__U16STRING) || !defined(HAVE_STD__U32STRING) 
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif // defined(HAVE_INTTYPES_H)

#ifndef HAVE_STD__U16STRING
#ifndef HAVE_CHAR16_T
typedef uint_least16_t char16_t;
#endif // HAVE_CHAR16_T
namespace std
{
  typedef basic_string<char16_t> u16string;
}
#endif // HAVE_STD__U16STRING

#ifndef HAVE_STD__U32STRING
#ifndef HAVE_CHAR32_T
typedef uint_least32_t char32_t;
#endif // HAVE_CHAR32_T
namespace std
{
  typedef basic_string<char32_t> u32string;
}
#endif // HAVE_STD__U32STRING

#endif // !defined(HAVE_STD__U16STRING) || !defined(HAVE_STD__U32STRING) 
#endif // TARGET_WINDOWS

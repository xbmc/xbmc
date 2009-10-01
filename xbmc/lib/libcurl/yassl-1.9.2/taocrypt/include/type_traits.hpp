/* type_traits.hpp                                
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

/* type_traits defines fundamental types
 * see discussion in C++ Templates, $19.1
*/


#ifndef TAO_CRYPT_TYPE_TRAITS_HPP
#define TAO_CRYPT_TYPE_TRAITS_HPP

#include "types.hpp"

namespace TaoCrypt {


// primary template: in general T is not a fundamental type

template <typename T>
class IsFundamentalType {
    public:
        enum { Yes = 0, No = 1 };
};


// macro to specialize for fundamental types
#define MK_FUNDAMENTAL_TYPE(T)                  \
    template<> class IsFundamentalType<T> {     \
        public:                                 \
            enum { Yes = 1, No = 0 };           \
    };


MK_FUNDAMENTAL_TYPE(void)

MK_FUNDAMENTAL_TYPE(bool)
MK_FUNDAMENTAL_TYPE(         char)
MK_FUNDAMENTAL_TYPE(signed   char)
MK_FUNDAMENTAL_TYPE(unsigned char)

MK_FUNDAMENTAL_TYPE(signed   short)
MK_FUNDAMENTAL_TYPE(unsigned short)
MK_FUNDAMENTAL_TYPE(signed   int)
MK_FUNDAMENTAL_TYPE(unsigned int)
MK_FUNDAMENTAL_TYPE(signed   long)
MK_FUNDAMENTAL_TYPE(unsigned long)

MK_FUNDAMENTAL_TYPE(float)
MK_FUNDAMENTAL_TYPE(     double)
MK_FUNDAMENTAL_TYPE(long double)

#if defined(WORD64_AVAILABLE) && defined(WORD64_IS_DISTINCT_TYPE)
    MK_FUNDAMENTAL_TYPE(word64)
#endif


#undef MK_FUNDAMENTAL_TYPE


} // namespace

#endif // TAO_CRYPT_TYPE_TRAITS_HPP

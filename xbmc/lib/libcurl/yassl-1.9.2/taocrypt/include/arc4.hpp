/* arc4.hpp                                
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

/* arc4.hpp defines ARC4
*/


#ifndef TAO_CRYPT_ARC4_HPP
#define TAO_CRYPT_ARC4_HPP

#include "misc.hpp"

namespace TaoCrypt {


// ARC4 encryption and decryption
class ARC4 {
public:
    enum { STATE_SIZE = 256 };

    typedef ARC4 Encryption;
    typedef ARC4 Decryption;

    ARC4() {}

    void Process(byte*, const byte*, word32);
    void SetKey(const byte*, word32);
private:
    byte x_;
    byte y_;
    byte state_[STATE_SIZE];

    ARC4(const ARC4&);                  // hide copy
    const ARC4 operator=(const ARC4&);  // and assign

    void AsmProcess(byte*, const byte*, word32);
};

} // namespace


#endif // TAO_CRYPT_ARC4_HPP


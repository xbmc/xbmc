/* dh.hpp                                
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

/* dh.hpp provides Diffie-Hellman support
*/


#ifndef TAO_CRYPT_DH_HPP
#define TAO_CRYPT_DH_HPP

#include "misc.hpp"
#include "integer.hpp"

namespace TaoCrypt {


class Source;


// Diffie-Hellman
class DH {
public:
    DH() {}
    DH(Integer& p, Integer& g) : p_(p), g_(g) {}
    explicit DH(Source&);

    DH(const DH& that) : p_(that.p_), g_(that.g_) {}
    DH& operator=(const DH& that) 
    {
        DH tmp(that);
        Swap(tmp);
        return *this;
    }

    void Swap(DH& other)
    {
        p_.Swap(other.p_);
        g_.Swap(other.g_);
    }

    void Initialize(Source&);
    void Initialize(Integer& p, Integer& g)
    {
        SetP(p);
        SetG(g);
    }

    void GenerateKeyPair(RandomNumberGenerator&, byte*, byte*);
    void Agree(byte*, const byte*, const byte*, word32 otherSz = 0);

    void SetP(const Integer& p) { p_ = p; }
    void SetG(const Integer& g) { g_ = g; }

    Integer& GetP() { return p_; }
    Integer& GetG() { return g_; }

    // for p and agree
    word32 GetByteLength() const { return p_.ByteCount(); }
private:
    // group parms
    Integer p_;
    Integer g_;

    void GeneratePrivate(RandomNumberGenerator&, byte*);
    void GeneratePublic(const byte*, byte*);    
};


} // namespace

#endif // TAO_CRYPT_DH_HPP

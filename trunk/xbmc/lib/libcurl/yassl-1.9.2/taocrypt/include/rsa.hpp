/* rsa.hpp                                
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

/* rsa.hpp provides RSA ES encrypt/decrypt, SSL (block type 1) sign and verify
*/

#ifndef TAO_CRYPT_RSA_HPP
#define TAO_CRYPT_RSA_HPP

#include "integer.hpp"
#include "random.hpp"


namespace TaoCrypt {

class Source;


// Public Key Length helper
class PK_Lengths {
    const Integer& image_;
public:
    explicit PK_Lengths(const Integer& i) : image_(i) {}

    word32 PaddedBlockBitLength()  const {return image_.BitCount() - 1;}
    word32 PaddedBlockByteLength() const 
                {return BitsToBytes(PaddedBlockBitLength());}

    word32 FixedCiphertextLength()   const {return image_.ByteCount();}
    word32 FixedMaxPlaintextLength() const 
                {return SaturatingSubtract(PaddedBlockBitLength() / 8, 10U); }
};


// RSA Public Key
class RSA_PublicKey {
protected:
    Integer n_;
    Integer e_;
public:
    RSA_PublicKey() {}
    explicit RSA_PublicKey(Source&);

    void Initialize(const Integer& n, const Integer& e) {n_ = n; e_ = e;}
    void Initialize(Source&);

    Integer ApplyFunction(const Integer& x) const;

    const Integer& GetModulus() const {return n_;}
    const Integer& GetPublicExponent() const {return e_;}

    void SetModulus(const Integer& n) {n_ = n;}
    void SetPublicExponent(const Integer& e) {e_ = e;}

    word32 FixedCiphertextLength()
    {
        return PK_Lengths(n_).FixedCiphertextLength();
    }

    RSA_PublicKey(const RSA_PublicKey& other) : n_(other.n_), e_(other.e_) {}
    RSA_PublicKey& operator=(const RSA_PublicKey& that)
    {
        RSA_PublicKey tmp(that);
        Swap(tmp);
        return *this;
    }

    void Swap(RSA_PublicKey& other)
    {
        n_.Swap(other.n_);
        e_.Swap(other.e_);
    }
};


// RSA Private Key
class RSA_PrivateKey : public RSA_PublicKey {
    Integer d_;
    Integer p_;
    Integer q_;
    Integer dp_;
    Integer dq_;
    Integer u_;
public:
    RSA_PrivateKey() {}
    explicit RSA_PrivateKey(Source&);

    void Initialize(const Integer& n,  const Integer& e, const Integer& d,
                    const Integer& p,  const Integer& q, const Integer& dp, 
                    const Integer& dq, const Integer& u)
        {n_ = n; e_ = e; d_ = d; p_ = p; q_ = q; dp_ = dp; dq_ = dq; u_ = u;}
    void Initialize(Source&);

    Integer CalculateInverse(RandomNumberGenerator&, const Integer&) const;

    const Integer& GetPrime1() const {return p_;}
    const Integer& GetPrime2() const {return q_;}
    const Integer& GetPrivateExponent() const {return d_;}
    const Integer& GetModPrime1PrivateExponent() const {return dp_;}
    const Integer& GetModPrime2PrivateExponent() const {return dq_;}
    const Integer& GetMultiplicativeInverseOfPrime2ModPrime1() const 
                   {return u_;}

    void SetPrime1(const Integer& p) {p_ = p;}
    void SetPrime2(const Integer& q) {q_ = q;}
    void SetPrivateExponent(const Integer& d) {d_ = d;}
    void SetModPrime1PrivateExponent(const Integer& dp) {dp_ = dp;}
    void SetModPrime2PrivateExponent(const Integer& dq) {dq_ = dq;}
    void SetMultiplicativeInverseOfPrime2ModPrime1(const Integer& u) {u_ = u;}
private:
    RSA_PrivateKey(const RSA_PrivateKey&);              // hide copy
    RSA_PrivateKey& operator=(const RSA_PrivateKey&);   // and assign
};


// block type 2 padding
class RSA_BlockType2  {
public:
    void   Pad(const byte*, word32, byte*, word32,
               RandomNumberGenerator&) const;
    word32 UnPad(const byte*, word32, byte*) const;
};


// block type 1 padding
class RSA_BlockType1  {
public:
    void   Pad(const byte*, word32, byte*, word32, 
               RandomNumberGenerator&) const;
    word32 UnPad(const byte*, word32, byte*) const;
};


// RSA Encryptor, can use any padding
template<class Pad = RSA_BlockType2>
class RSA_Encryptor {
    const RSA_PublicKey& key_;
    Pad                  padding_;
public:
    explicit RSA_Encryptor(const RSA_PublicKey& k) : key_(k) {}

    void Encrypt(const byte*, word32, byte*, RandomNumberGenerator&);
    bool SSL_Verify(const byte* msg, word32 sz, const byte* sig);
};


// RSA Decryptor, can use any padding
template<class Pad = RSA_BlockType2>
class RSA_Decryptor {
    const RSA_PrivateKey& key_;
    Pad                   padding_;
public:
    explicit RSA_Decryptor(const RSA_PrivateKey& k) : key_(k) {}

    word32 Decrypt(const byte*, word32, byte*, RandomNumberGenerator&);
    void   SSL_Sign(const byte*, word32, byte*, RandomNumberGenerator&);
};


// Public Encrypt
template<class Pad>
void RSA_Encryptor<Pad>::Encrypt(const byte* plain, word32 sz, byte* cipher,
                                 RandomNumberGenerator& rng)
{
    PK_Lengths lengths(key_.GetModulus());
    assert(sz <= lengths.FixedMaxPlaintextLength());

    ByteBlock paddedBlock(lengths.PaddedBlockByteLength());
    padding_.Pad(plain, sz, paddedBlock.get_buffer(),
                 lengths.PaddedBlockBitLength(), rng);

    key_.ApplyFunction(Integer(paddedBlock.get_buffer(), paddedBlock.size())).
        Encode(cipher, lengths.FixedCiphertextLength());
}


// Private Decrypt
template<class Pad>
word32 RSA_Decryptor<Pad>::Decrypt(const byte* cipher, word32 sz, byte* plain,
                                   RandomNumberGenerator& rng)
{
    PK_Lengths lengths(key_.GetModulus());
    assert(sz == lengths.FixedCiphertextLength());

    if (sz != lengths.FixedCiphertextLength())
        return 0;
       
    ByteBlock paddedBlock(lengths.PaddedBlockByteLength());
    Integer x = key_.CalculateInverse(rng, Integer(cipher,
                                      lengths.FixedCiphertextLength()).Ref());
    if (x.ByteCount() > paddedBlock.size())
        x = Integer::Zero();	// don't return false, prevents timing attack
    x.Encode(paddedBlock.get_buffer(), paddedBlock.size());
    return padding_.UnPad(paddedBlock.get_buffer(),
                          lengths.PaddedBlockBitLength(), plain);
}


// Private SSL type (block 1) Encrypt
template<class Pad>
void RSA_Decryptor<Pad>::SSL_Sign(const byte* message, word32 sz, byte* sig,
                                  RandomNumberGenerator& rng)
{
    RSA_PublicKey inverse;
    inverse.Initialize(key_.GetModulus(), key_.GetPrivateExponent());
    RSA_Encryptor<RSA_BlockType1> enc(inverse); // SSL Type
    enc.Encrypt(message, sz, sig, rng);
}


word32 SSL_Decrypt(const RSA_PublicKey& key, const byte* sig, byte* plain);


// Public SSL type (block 1) Decrypt
template<class Pad>
bool RSA_Encryptor<Pad>::SSL_Verify(const byte* message, word32 sz,
                                    const byte* sig)
{
    ByteBlock plain(PK_Lengths(key_.GetModulus()).FixedMaxPlaintextLength());
    if (SSL_Decrypt(key_, sig, plain.get_buffer()) != sz)
        return false;   // not right justified or bad padding

    if ( (memcmp(plain.get_buffer(), message, sz)) == 0)
        return true;
    return false;
}


typedef RSA_Encryptor<> RSAES_Encryptor;
typedef RSA_Decryptor<> RSAES_Decryptor;


} // namespace

#endif // TAO_CRYPT_RSA_HPP

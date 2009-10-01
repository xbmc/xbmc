/* crypto_wrapper.cpp  
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

/*  The crypto wrapper source implements the policies for the cipher
 *  components used by SSL.
 *
 *  The implementation relies on a specfic library, taoCrypt.
 */

#if !defined(USE_CRYPTOPP_LIB)

#include "runtime.hpp"
#include "crypto_wrapper.hpp"
#include "cert_wrapper.hpp"

#include "md5.hpp"
#include "sha.hpp"
#include "ripemd.hpp"
#include "hmac.hpp"
#include "modes.hpp"
#include "des.hpp"
#include "arc4.hpp"
#include "aes.hpp"
#include "rsa.hpp"
#include "dsa.hpp"
#include "dh.hpp"
#include "random.hpp"
#include "file.hpp"
#include "coding.hpp"


namespace yaSSL {


// MD5 Implementation
struct MD5::MD5Impl {
    TaoCrypt::MD5 md5_;
    MD5Impl() {}
    explicit MD5Impl(const TaoCrypt::MD5& md5) : md5_(md5) {}
};


MD5::MD5() : pimpl_(NEW_YS MD5Impl) {}


MD5::~MD5() { ysDelete(pimpl_); }


MD5::MD5(const MD5& that) : Digest(), pimpl_(NEW_YS 
                                             MD5Impl(that.pimpl_->md5_)) {}


MD5& MD5::operator=(const MD5& that)
{
    pimpl_->md5_ = that.pimpl_->md5_;
    return *this;
}


uint MD5::get_digestSize() const
{
    return MD5_LEN;
}


uint MD5::get_padSize() const
{
    return PAD_MD5;
}


// Fill out with MD5 digest from in that is sz bytes, out must be >= digest sz
void MD5::get_digest(byte* out, const byte* in, unsigned int sz)
{
    pimpl_->md5_.Update(in, sz);
    pimpl_->md5_.Final(out);
}

// Fill out with MD5 digest from previous updates
void MD5::get_digest(byte* out)
{
    pimpl_->md5_.Final(out);
}


// Update the current digest
void MD5::update(const byte* in, unsigned int sz)
{
    pimpl_->md5_.Update(in, sz);
}


// SHA Implementation
struct SHA::SHAImpl {
    TaoCrypt::SHA sha_;
    SHAImpl() {}
    explicit SHAImpl(const TaoCrypt::SHA& sha) : sha_(sha) {}
};


SHA::SHA() : pimpl_(NEW_YS SHAImpl) {}


SHA::~SHA() { ysDelete(pimpl_); }


SHA::SHA(const SHA& that) : Digest(), pimpl_(NEW_YS SHAImpl(that.pimpl_->sha_)) {}

SHA& SHA::operator=(const SHA& that)
{
    pimpl_->sha_ = that.pimpl_->sha_;
    return *this;
}


uint SHA::get_digestSize() const
{
    return SHA_LEN;
}


uint SHA::get_padSize() const
{
    return PAD_SHA;
}


// Fill out with SHA digest from in that is sz bytes, out must be >= digest sz
void SHA::get_digest(byte* out, const byte* in, unsigned int sz)
{
    pimpl_->sha_.Update(in, sz);
    pimpl_->sha_.Final(out);
}


// Fill out with SHA digest from previous updates
void SHA::get_digest(byte* out)
{
    pimpl_->sha_.Final(out);
}


// Update the current digest
void SHA::update(const byte* in, unsigned int sz)
{
    pimpl_->sha_.Update(in, sz);
}


// RMD-160 Implementation
struct RMD::RMDImpl {
    TaoCrypt::RIPEMD160 rmd_;
    RMDImpl() {}
    explicit RMDImpl(const TaoCrypt::RIPEMD160& rmd) : rmd_(rmd) {}
};


RMD::RMD() : pimpl_(NEW_YS RMDImpl) {}


RMD::~RMD() { ysDelete(pimpl_); }


RMD::RMD(const RMD& that) : Digest(), pimpl_(NEW_YS RMDImpl(that.pimpl_->rmd_)) {}

RMD& RMD::operator=(const RMD& that)
{
    pimpl_->rmd_ = that.pimpl_->rmd_;
    return *this;
}


uint RMD::get_digestSize() const
{
    return RMD_LEN;
}


uint RMD::get_padSize() const
{
    return PAD_RMD;
}


// Fill out with RMD digest from in that is sz bytes, out must be >= digest sz
void RMD::get_digest(byte* out, const byte* in, unsigned int sz)
{
    pimpl_->rmd_.Update(in, sz);
    pimpl_->rmd_.Final(out);
}


// Fill out with RMD digest from previous updates
void RMD::get_digest(byte* out)
{
    pimpl_->rmd_.Final(out);
}


// Update the current digest
void RMD::update(const byte* in, unsigned int sz)
{
    pimpl_->rmd_.Update(in, sz);
}


// HMAC_MD5 Implementation
struct HMAC_MD5::HMAC_MD5Impl {
    TaoCrypt::HMAC<TaoCrypt::MD5> mac_;
    HMAC_MD5Impl() {}
};


HMAC_MD5::HMAC_MD5(const byte* secret, unsigned int len) 
    : pimpl_(NEW_YS HMAC_MD5Impl) 
{
    pimpl_->mac_.SetKey(secret, len);
}


HMAC_MD5::~HMAC_MD5() { ysDelete(pimpl_); }


uint HMAC_MD5::get_digestSize() const
{
    return MD5_LEN;
}


uint HMAC_MD5::get_padSize() const
{
    return PAD_MD5;
}


// Fill out with MD5 digest from in that is sz bytes, out must be >= digest sz
void HMAC_MD5::get_digest(byte* out, const byte* in, unsigned int sz)
{
    pimpl_->mac_.Update(in, sz);
    pimpl_->mac_.Final(out);
}

// Fill out with MD5 digest from previous updates
void HMAC_MD5::get_digest(byte* out)
{
    pimpl_->mac_.Final(out);
}


// Update the current digest
void HMAC_MD5::update(const byte* in, unsigned int sz)
{
    pimpl_->mac_.Update(in, sz);
}


// HMAC_SHA Implementation
struct HMAC_SHA::HMAC_SHAImpl {
    TaoCrypt::HMAC<TaoCrypt::SHA> mac_;
    HMAC_SHAImpl() {}
};


HMAC_SHA::HMAC_SHA(const byte* secret, unsigned int len) 
    : pimpl_(NEW_YS HMAC_SHAImpl) 
{
    pimpl_->mac_.SetKey(secret, len);
}


HMAC_SHA::~HMAC_SHA() { ysDelete(pimpl_); }


uint HMAC_SHA::get_digestSize() const
{
    return SHA_LEN;
}


uint HMAC_SHA::get_padSize() const
{
    return PAD_SHA;
}


// Fill out with SHA digest from in that is sz bytes, out must be >= digest sz
void HMAC_SHA::get_digest(byte* out, const byte* in, unsigned int sz)
{
    pimpl_->mac_.Update(in, sz);
    pimpl_->mac_.Final(out);
}

// Fill out with SHA digest from previous updates
void HMAC_SHA::get_digest(byte* out)
{
    pimpl_->mac_.Final(out);
}


// Update the current digest
void HMAC_SHA::update(const byte* in, unsigned int sz)
{
    pimpl_->mac_.Update(in, sz);
}



// HMAC_RMD Implementation
struct HMAC_RMD::HMAC_RMDImpl {
    TaoCrypt::HMAC<TaoCrypt::RIPEMD160> mac_;
    HMAC_RMDImpl() {}
};


HMAC_RMD::HMAC_RMD(const byte* secret, unsigned int len) 
    : pimpl_(NEW_YS HMAC_RMDImpl) 
{
    pimpl_->mac_.SetKey(secret, len);
}


HMAC_RMD::~HMAC_RMD() { ysDelete(pimpl_); }


uint HMAC_RMD::get_digestSize() const
{
    return RMD_LEN;
}


uint HMAC_RMD::get_padSize() const
{
    return PAD_RMD;
}


// Fill out with RMD digest from in that is sz bytes, out must be >= digest sz
void HMAC_RMD::get_digest(byte* out, const byte* in, unsigned int sz)
{
    pimpl_->mac_.Update(in, sz);
    pimpl_->mac_.Final(out);
}

// Fill out with RMD digest from previous updates
void HMAC_RMD::get_digest(byte* out)
{
    pimpl_->mac_.Final(out);
}


// Update the current digest
void HMAC_RMD::update(const byte* in, unsigned int sz)
{
    pimpl_->mac_.Update(in, sz);
}


struct DES::DESImpl {
    TaoCrypt::DES_CBC_Encryption encryption;
    TaoCrypt::DES_CBC_Decryption decryption;
};


DES::DES() : pimpl_(NEW_YS DESImpl) {}

DES::~DES() { ysDelete(pimpl_); }


void DES::set_encryptKey(const byte* k, const byte* iv)
{
    pimpl_->encryption.SetKey(k, DES_KEY_SZ, iv);
}


void DES::set_decryptKey(const byte* k, const byte* iv)
{
    pimpl_->decryption.SetKey(k, DES_KEY_SZ, iv);
}

// DES encrypt plain of length sz into cipher
void DES::encrypt(byte* cipher, const byte* plain, unsigned int sz)
{
    pimpl_->encryption.Process(cipher, plain, sz);
}


// DES decrypt cipher of length sz into plain
void DES::decrypt(byte* plain, const byte* cipher, unsigned int sz)
{
    pimpl_->decryption.Process(plain, cipher, sz);
}


struct DES_EDE::DES_EDEImpl {
    TaoCrypt::DES_EDE3_CBC_Encryption encryption;
    TaoCrypt::DES_EDE3_CBC_Decryption decryption;
};


DES_EDE::DES_EDE() : pimpl_(NEW_YS DES_EDEImpl) {}

DES_EDE::~DES_EDE() { ysDelete(pimpl_); }


void DES_EDE::set_encryptKey(const byte* k, const byte* iv)
{
    pimpl_->encryption.SetKey(k, DES_EDE_KEY_SZ, iv);
}


void DES_EDE::set_decryptKey(const byte* k, const byte* iv)
{
    pimpl_->decryption.SetKey(k, DES_EDE_KEY_SZ, iv);
}


// 3DES encrypt plain of length sz into cipher
void DES_EDE::encrypt(byte* cipher, const byte* plain, unsigned int sz)
{
    pimpl_->encryption.Process(cipher, plain, sz);
}


// 3DES decrypt cipher of length sz into plain
void DES_EDE::decrypt(byte* plain, const byte* cipher, unsigned int sz)
{
    pimpl_->decryption.Process(plain, cipher, sz);
}


// Implementation of alledged RC4
struct RC4::RC4Impl {
    TaoCrypt::ARC4::Encryption encryption;
    TaoCrypt::ARC4::Decryption decryption;
};


RC4::RC4() : pimpl_(NEW_YS RC4Impl) {}

RC4::~RC4() { ysDelete(pimpl_); }


void RC4::set_encryptKey(const byte* k, const byte*)
{
    pimpl_->encryption.SetKey(k, RC4_KEY_SZ);
}


void RC4::set_decryptKey(const byte* k, const byte*)
{
    pimpl_->decryption.SetKey(k, RC4_KEY_SZ);
}


// RC4 encrypt plain of length sz into cipher
void RC4::encrypt(byte* cipher, const byte* plain, unsigned int sz)
{
    pimpl_->encryption.Process(cipher, plain, sz);
}


// RC4 decrypt cipher of length sz into plain
void RC4::decrypt(byte* plain, const byte* cipher, unsigned int sz)
{
    pimpl_->decryption.Process(plain, cipher, sz);
}



// Implementation of AES
struct AES::AESImpl {
    TaoCrypt::AES_CBC_Encryption encryption;
    TaoCrypt::AES_CBC_Decryption decryption;
    unsigned int keySz_;

    AESImpl(unsigned int ks) : keySz_(ks) {}
};


AES::AES(unsigned int ks) : pimpl_(NEW_YS AESImpl(ks)) {}

AES::~AES() { ysDelete(pimpl_); }


int AES::get_keySize() const
{
    return pimpl_->keySz_;
}


void AES::set_encryptKey(const byte* k, const byte* iv)
{
    pimpl_->encryption.SetKey(k, pimpl_->keySz_, iv);
}


void AES::set_decryptKey(const byte* k, const byte* iv)
{
    pimpl_->decryption.SetKey(k, pimpl_->keySz_, iv);
}


// AES encrypt plain of length sz into cipher
void AES::encrypt(byte* cipher, const byte* plain, unsigned int sz)
{
    pimpl_->encryption.Process(cipher, plain, sz);
}


// AES decrypt cipher of length sz into plain
void AES::decrypt(byte* plain, const byte* cipher, unsigned int sz)
{
    pimpl_->decryption.Process(plain, cipher, sz);
}


struct RandomPool::RandomImpl {
    TaoCrypt::RandomNumberGenerator RNG_;
};

RandomPool::RandomPool() : pimpl_(NEW_YS RandomImpl) {}

RandomPool::~RandomPool() { ysDelete(pimpl_); }

int RandomPool::GetError() const
{
    return pimpl_->RNG_.GetError(); 
}

void RandomPool::Fill(opaque* dst, uint sz) const
{
    pimpl_->RNG_.GenerateBlock(dst, sz);
}


// Implementation of DSS Authentication
struct DSS::DSSImpl {
    void SetPublic (const byte*, unsigned int);
    void SetPrivate(const byte*, unsigned int);
    TaoCrypt::DSA_PublicKey publicKey_;
    TaoCrypt::DSA_PrivateKey privateKey_;
};


// Decode and store the public key
void DSS::DSSImpl::SetPublic(const byte* key, unsigned int sz)
{
    TaoCrypt::Source source(key, sz);
    publicKey_.Initialize(source);
}


// Decode and store the public key
void DSS::DSSImpl::SetPrivate(const byte* key, unsigned int sz)
{
    TaoCrypt::Source source(key, sz);
    privateKey_.Initialize(source);
    publicKey_ = TaoCrypt::DSA_PublicKey(privateKey_);

}


// Set public or private key
DSS::DSS(const byte* key, unsigned int sz, bool publicKey) 
    : pimpl_(NEW_YS DSSImpl)
{
    if (publicKey) 
        pimpl_->SetPublic(key, sz);
    else
        pimpl_->SetPrivate(key, sz);
}


DSS::~DSS()
{
    ysDelete(pimpl_);
}


uint DSS::get_signatureLength() const
{
    return pimpl_->publicKey_.SignatureLength();
}


// DSS Sign message of length sz into sig
void DSS::sign(byte* sig,  const byte* sha_digest, unsigned int /* shaSz */,
               const RandomPool& random)
{
    using namespace TaoCrypt;

    DSA_Signer signer(pimpl_->privateKey_);
    signer.Sign(sha_digest, sig, random.pimpl_->RNG_);
}


// DSS Verify message of length sz against sig, is it correct?
bool DSS::verify(const byte* sha_digest, unsigned int /* shaSz */,
                 const byte* sig, unsigned int /* sigSz */)
{
    using namespace TaoCrypt;

    DSA_Verifier ver(pimpl_->publicKey_);
    return ver.Verify(sha_digest, sig);
}


// Implementation of RSA key interface
struct RSA::RSAImpl {
    void SetPublic (const byte*, unsigned int);
    void SetPrivate(const byte*, unsigned int);
    TaoCrypt::RSA_PublicKey publicKey_;
    TaoCrypt::RSA_PrivateKey privateKey_;
};


// Decode and store the public key
void RSA::RSAImpl::SetPublic(const byte* key, unsigned int sz)
{
    TaoCrypt::Source source(key, sz);
    publicKey_.Initialize(source);
}


// Decode and store the private key
void RSA::RSAImpl::SetPrivate(const byte* key, unsigned int sz)
{
    TaoCrypt::Source source(key, sz);
    privateKey_.Initialize(source);
    publicKey_ = TaoCrypt::RSA_PublicKey(privateKey_);
}


// Set public or private key
RSA::RSA(const byte* key, unsigned int sz, bool publicKey) 
    : pimpl_(NEW_YS RSAImpl)
{
    if (publicKey) 
        pimpl_->SetPublic(key, sz);
    else
        pimpl_->SetPrivate(key, sz);
}

RSA::~RSA()
{
    ysDelete(pimpl_);
}


// get cipher text length, varies on key size
unsigned int RSA::get_cipherLength() const
{
    return pimpl_->publicKey_.FixedCiphertextLength();
}


// get signautre length, varies on key size
unsigned int RSA::get_signatureLength() const
{
    return get_cipherLength();
}


// RSA Sign message of length sz into sig
void RSA::sign(byte* sig,  const byte* message, unsigned int sz,
               const RandomPool& random)
{
    TaoCrypt::RSAES_Decryptor dec(pimpl_->privateKey_);
    dec.SSL_Sign(message, sz, sig, random.pimpl_->RNG_);
}


// RSA Verify message of length sz against sig
bool RSA::verify(const byte* message, unsigned int sz, const byte* sig,
                 unsigned int)
{
    TaoCrypt::RSAES_Encryptor enc(pimpl_->publicKey_);
    return enc.SSL_Verify(message, sz, sig);
}


// RSA public encrypt plain of length sz into cipher
void RSA::encrypt(byte* cipher, const byte* plain, unsigned int sz,
                  const RandomPool& random)
{
  
    TaoCrypt::RSAES_Encryptor enc(pimpl_->publicKey_);
    enc.Encrypt(plain, sz, cipher, random.pimpl_->RNG_);
}


// RSA private decrypt cipher of length sz into plain
void RSA::decrypt(byte* plain, const byte* cipher, unsigned int sz,
                  const RandomPool& random)
{
    TaoCrypt::RSAES_Decryptor dec(pimpl_->privateKey_);
    dec.Decrypt(cipher, sz, plain, random.pimpl_->RNG_);
}


struct Integer::IntegerImpl {
    TaoCrypt::Integer int_;

    IntegerImpl() {}
    explicit IntegerImpl(const TaoCrypt::Integer& i) : int_(i) {}
};

Integer::Integer() : pimpl_(NEW_YS IntegerImpl) {}

Integer::~Integer() { ysDelete(pimpl_); }



Integer::Integer(const Integer& other) : pimpl_(NEW_YS 
                                               IntegerImpl(other.pimpl_->int_))
{}


Integer& Integer::operator=(const Integer& that)
{
    pimpl_->int_ = that.pimpl_->int_;

    return *this;
}


void Integer::assign(const byte* num, unsigned int sz)
{
    pimpl_->int_ = TaoCrypt::Integer(num, sz);
}


struct DiffieHellman::DHImpl {
    TaoCrypt::DH                     dh_;
    TaoCrypt::RandomNumberGenerator& ranPool_;
    byte* publicKey_;
    byte* privateKey_;
    byte* agreedKey_;

    DHImpl(TaoCrypt::RandomNumberGenerator& r) : ranPool_(r), publicKey_(0),
                                               privateKey_(0), agreedKey_(0) {}
    ~DHImpl() 
    {   
        ysArrayDelete(agreedKey_); 
        ysArrayDelete(privateKey_); 
        ysArrayDelete(publicKey_);
    }

    DHImpl(const DHImpl& that) : dh_(that.dh_), ranPool_(that.ranPool_),
                                 publicKey_(0), privateKey_(0), agreedKey_(0)
    {
        uint length = dh_.GetByteLength();
        AllocKeys(length, length, length);
    }

    void AllocKeys(unsigned int pubSz, unsigned int privSz, unsigned int agrSz)
    {
        publicKey_  = NEW_YS byte[pubSz];
        privateKey_ = NEW_YS byte[privSz];
        agreedKey_  = NEW_YS byte[agrSz];
    }
};



/*
// server Side DH, server's view
DiffieHellman::DiffieHellman(const char* file, const RandomPool& random)
    : pimpl_(NEW_YS DHImpl(random.pimpl_->RNG_))
{
    using namespace TaoCrypt;
    Source source;
    FileSource(file, source);
    if (source.size() == 0)
        return; // TODO add error state, and force check
    HexDecoder hd(source);

    pimpl_->dh_.Initialize(source);

    uint length = pimpl_->dh_.GetByteLength();

    pimpl_->AllocKeys(length, length, length);
    pimpl_->dh_.GenerateKeyPair(pimpl_->ranPool_, pimpl_->privateKey_,
                                                  pimpl_->publicKey_);
}
*/


// server Side DH, client's view
DiffieHellman::DiffieHellman(const byte* p, unsigned int pSz, const byte* g,
                             unsigned int gSz, const byte* pub,
                             unsigned int pubSz, const RandomPool& random)
    : pimpl_(NEW_YS DHImpl(random.pimpl_->RNG_))
{
    using TaoCrypt::Integer;

    pimpl_->dh_.Initialize(Integer(p, pSz).Ref(), Integer(g, gSz).Ref());
    pimpl_->publicKey_ = NEW_YS opaque[pubSz];
    memcpy(pimpl_->publicKey_, pub, pubSz);
}


// Server Side DH, server's view
DiffieHellman::DiffieHellman(const Integer& p, const Integer& g,
                             const RandomPool& random)
: pimpl_(NEW_YS DHImpl(random.pimpl_->RNG_))
{
    using TaoCrypt::Integer;

    pimpl_->dh_.Initialize(p.pimpl_->int_, g.pimpl_->int_);

    uint length = pimpl_->dh_.GetByteLength();

    pimpl_->AllocKeys(length, length, length);
    pimpl_->dh_.GenerateKeyPair(pimpl_->ranPool_, pimpl_->privateKey_,
                                                  pimpl_->publicKey_);
}

DiffieHellman::~DiffieHellman() { ysDelete(pimpl_); }


// Client side and view, use server that for p and g
DiffieHellman::DiffieHellman(const DiffieHellman& that) 
    : pimpl_(NEW_YS DHImpl(*that.pimpl_))
{   
    pimpl_->dh_.GenerateKeyPair(pimpl_->ranPool_, pimpl_->privateKey_,
                                                  pimpl_->publicKey_);
}


DiffieHellman& DiffieHellman::operator=(const DiffieHellman& that)
{
    pimpl_->dh_ = that.pimpl_->dh_;
    pimpl_->dh_.GenerateKeyPair(pimpl_->ranPool_, pimpl_->privateKey_,
                                                  pimpl_->publicKey_);
    return *this;
}


void DiffieHellman::makeAgreement(const byte* other, unsigned int otherSz)
{
    pimpl_->dh_.Agree(pimpl_->agreedKey_, pimpl_->privateKey_, other, otherSz); 
}


uint DiffieHellman::get_agreedKeyLength() const
{
    return pimpl_->dh_.GetByteLength();
}


const byte* DiffieHellman::get_agreedKey() const
{
    return pimpl_->agreedKey_;
}


const byte* DiffieHellman::get_publicKey() const
{
    return pimpl_->publicKey_;
}


void DiffieHellman::set_sizes(int& pSz, int& gSz, int& pubSz) const
{
    using TaoCrypt::Integer;
    Integer p = pimpl_->dh_.GetP();
    Integer g = pimpl_->dh_.GetG();

    pSz   = p.ByteCount();
    gSz   = g.ByteCount();
    pubSz = pimpl_->dh_.GetByteLength();
}


void DiffieHellman::get_parms(byte* bp, byte* bg, byte* bpub) const
{
    using TaoCrypt::Integer;
    Integer p = pimpl_->dh_.GetP();
    Integer g = pimpl_->dh_.GetG();

    p.Encode(bp, p.ByteCount());
    g.Encode(bg, g.ByteCount());
    memcpy(bpub, pimpl_->publicKey_, pimpl_->dh_.GetByteLength());
}


// convert PEM file to DER x509 type
x509* PemToDer(FILE* file, CertType type, EncryptedInfo* info)
{
    using namespace TaoCrypt;

    char header[80];
    char footer[80];

    if (type == Cert) {
        strncpy(header, "-----BEGIN CERTIFICATE-----", sizeof(header));
        strncpy(footer, "-----END CERTIFICATE-----", sizeof(footer));
    } else {
        strncpy(header, "-----BEGIN RSA PRIVATE KEY-----", sizeof(header));
        strncpy(footer, "-----END RSA PRIVATE KEY-----", sizeof(header));
    }

    long begin = -1;
    long end   = 0;
    bool foundEnd = false;

    char line[80];

    while(fgets(line, sizeof(line), file))
        if (strncmp(header, line, strlen(header)) == 0) {
            begin = ftell(file);
            break;
        }

    // remove encrypted header if there
    if (fgets(line, sizeof(line), file)) {
        char encHeader[] = "Proc-Type";
        if (strncmp(encHeader, line, strlen(encHeader)) == 0 &&
            fgets(line,sizeof(line), file)) {

            char* start  = strstr(line, "DES");
            char* finish = strstr(line, ",");
            if (!start)
                start    = strstr(line, "AES");

            if (!info) return 0;

            if ( start && finish && (start < finish)) {
                memcpy(info->name, start, finish - start);
                info->name[finish - start] = 0;
                memcpy(info->iv, finish + 1, sizeof(info->iv));

                char* newline = strstr(line, "\r");
                if (!newline) newline = strstr(line, "\n");
                if (newline && (newline > finish)) {
                    info->ivSz = newline - (finish + 1);
                    info->set = true;
                }
            }
            fgets(line,sizeof(line), file); // get blank line
            begin = ftell(file);
        }
          
    }

    while(fgets(line, sizeof(line), file))
        if (strncmp(footer, line, strlen(footer)) == 0) {
            foundEnd = true;
            break;
        }
        else
            end = ftell(file);

    if (begin == -1 || !foundEnd)
        return 0;

    input_buffer tmp(end - begin);
    fseek(file, begin, SEEK_SET);
    size_t bytes = fread(tmp.get_buffer(), end - begin, 1, file);
    if (bytes != 1)
        return 0;
    
    Source der(tmp.get_buffer(), end - begin);
    Base64Decoder b64Dec(der);

    uint sz = der.size();
    mySTL::auto_ptr<x509> x(NEW_YS x509(sz));
    memcpy(x->use_buffer(), der.get_buffer(), sz);

    return x.release();
}


} // namespace


#ifdef HAVE_EXPLICIT_TEMPLATE_INSTANTIATION
namespace yaSSL {
template void ysDelete<DiffieHellman::DHImpl>(DiffieHellman::DHImpl*);
template void ysDelete<Integer::IntegerImpl>(Integer::IntegerImpl*);
template void ysDelete<RSA::RSAImpl>(RSA::RSAImpl*);
template void ysDelete<DSS::DSSImpl>(DSS::DSSImpl*);
template void ysDelete<RandomPool::RandomImpl>(RandomPool::RandomImpl*);
template void ysDelete<AES::AESImpl>(AES::AESImpl*);
template void ysDelete<RC4::RC4Impl>(RC4::RC4Impl*);
template void ysDelete<DES_EDE::DES_EDEImpl>(DES_EDE::DES_EDEImpl*);
template void ysDelete<DES::DESImpl>(DES::DESImpl*);
template void ysDelete<HMAC_RMD::HMAC_RMDImpl>(HMAC_RMD::HMAC_RMDImpl*);
template void ysDelete<HMAC_SHA::HMAC_SHAImpl>(HMAC_SHA::HMAC_SHAImpl*);
template void ysDelete<HMAC_MD5::HMAC_MD5Impl>(HMAC_MD5::HMAC_MD5Impl*);
template void ysDelete<RMD::RMDImpl>(RMD::RMDImpl*);
template void ysDelete<SHA::SHAImpl>(SHA::SHAImpl*);
template void ysDelete<MD5::MD5Impl>(MD5::MD5Impl*);
}
#endif // HAVE_EXPLICIT_TEMPLATE_INSTANTIATION

#endif // !USE_CRYPTOPP_LIB

/* crypto_wrapper.hpp                          
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


/*  The crypto wrapper header is used to define policies for the cipher 
 *  components used by SSL.  There are 3 policies to consider:
 *
 *  1) MAC, the Message Authentication Code used for each Message
 *  2) Bulk Cipher, the Cipher used to encrypt/decrypt each Message
 *  3) Atuhentication, the Digitial Signing/Verifiaction scheme used
 *
 *  This header doesn't rely on a specific crypto libraries internals,
 *  only the implementation should.
 */


#ifndef yaSSL_CRYPTO_WRAPPER_HPP
#define yaSSL_CRYPTO_WRAPPER_HPP

#include "yassl_types.hpp"
#include <stdio.h>   // FILE


namespace yaSSL {


// Digest policy should implement a get_digest, update, and get sizes for pad
// and  digest
struct Digest : public virtual_base {
    virtual void   get_digest(byte*) = 0;
    virtual void   get_digest(byte*, const byte*, unsigned int) = 0;
    virtual void   update(const byte*, unsigned int) = 0;
    virtual uint   get_digestSize() const = 0;
    virtual uint   get_padSize() const = 0;
    virtual ~Digest() {}
};


// For use with NULL Digests
struct NO_MAC : public Digest {
    void   get_digest(byte*);
    void   get_digest(byte*, const byte*, unsigned int);
    void   update(const byte*, unsigned int);
    uint   get_digestSize() const;
    uint   get_padSize()    const;
};


// MD5 Digest
class MD5 : public Digest {
public:
    void   get_digest(byte*);
    void   get_digest(byte*, const byte*, unsigned int);
    void   update(const byte*, unsigned int);
    uint   get_digestSize() const;
    uint   get_padSize()    const;
    MD5();
    ~MD5();
    MD5(const MD5&);
    MD5& operator=(const MD5&);
private:
    struct MD5Impl;
    MD5Impl* pimpl_;
};


// SHA-1 Digest
class SHA : public Digest {
public:
    void   get_digest(byte*);
    void   get_digest(byte*, const byte*, unsigned int);
    void   update(const byte*, unsigned int);
    uint   get_digestSize() const;
    uint   get_padSize()    const;
    SHA();
    ~SHA();
    SHA(const SHA&);
    SHA& operator=(const SHA&);
private:
    struct SHAImpl;
    SHAImpl* pimpl_;

};


// RIPEMD-160 Digest
class RMD : public Digest {
public:
    void   get_digest(byte*);
    void   get_digest(byte*, const byte*, unsigned int);
    void   update(const byte*, unsigned int);
    uint   get_digestSize() const;
    uint   get_padSize()    const;
    RMD();
    ~RMD();
    RMD(const RMD&);
    RMD& operator=(const RMD&);
private:
    struct RMDImpl;
    RMDImpl* pimpl_;

};


// HMAC_MD5
class HMAC_MD5 : public Digest {
public:
    void   get_digest(byte*);
    void   get_digest(byte*, const byte*, unsigned int);
    void   update(const byte*, unsigned int);
    uint   get_digestSize() const;
    uint   get_padSize()    const;
    HMAC_MD5(const byte*, unsigned int);
    ~HMAC_MD5();
private:
    struct HMAC_MD5Impl;
    HMAC_MD5Impl* pimpl_;

    HMAC_MD5(const HMAC_MD5&);
    HMAC_MD5& operator=(const HMAC_MD5&);
};


// HMAC_SHA-1
class HMAC_SHA : public Digest {
public:
    void   get_digest(byte*);
    void   get_digest(byte*, const byte*, unsigned int);
    void   update(const byte*, unsigned int);
    uint   get_digestSize() const;
    uint   get_padSize()    const;
    HMAC_SHA(const byte*, unsigned int);
    ~HMAC_SHA();
private:
    struct HMAC_SHAImpl;
    HMAC_SHAImpl* pimpl_;

    HMAC_SHA(const HMAC_SHA&);
    HMAC_SHA& operator=(const HMAC_SHA&);
};


// HMAC_RMD
class HMAC_RMD : public Digest {
public:
    void   get_digest(byte*);
    void   get_digest(byte*, const byte*, unsigned int);
    void   update(const byte*, unsigned int);
    uint   get_digestSize() const;
    uint   get_padSize()    const;
    HMAC_RMD(const byte*, unsigned int);
    ~HMAC_RMD();
private:
    struct HMAC_RMDImpl;
    HMAC_RMDImpl* pimpl_;

    HMAC_RMD(const HMAC_RMD&);
    HMAC_RMD& operator=(const HMAC_RMD&);
};


// BulkCipher policy should implement encrypt, decrypt, get block size, 
// and set keys for encrypt and decrypt
struct BulkCipher : public virtual_base {
    virtual void   encrypt(byte*, const byte*, unsigned int) = 0;
    virtual void   decrypt(byte*, const byte*, unsigned int) = 0;
    virtual void   set_encryptKey(const byte*, const byte* = 0) = 0;
    virtual void   set_decryptKey(const byte*, const byte* = 0) = 0;
    virtual uint   get_blockSize() const = 0;
    virtual int    get_keySize()   const = 0;
    virtual int    get_ivSize()    const = 0;
    virtual ~BulkCipher() {}
};


// For use with NULL Ciphers
struct NO_Cipher : public BulkCipher {
    void   encrypt(byte*, const byte*, unsigned int) {}
    void   decrypt(byte*, const byte*, unsigned int) {}
    void   set_encryptKey(const byte*, const byte*)  {}
    void   set_decryptKey(const byte*, const byte*)  {}
    uint   get_blockSize() const { return 0; }
    int    get_keySize()   const { return 0; }
    int    get_ivSize()    const { return 0; }
};


// SSLv3 and TLSv1 always use DES in CBC mode so IV is required
class DES : public BulkCipher {
public:
    void   encrypt(byte*, const byte*, unsigned int);
    void   decrypt(byte*, const byte*, unsigned int);
    void   set_encryptKey(const byte*, const byte*);
    void   set_decryptKey(const byte*, const byte*);
    uint   get_blockSize() const { return DES_BLOCK; }
    int    get_keySize()   const { return DES_KEY_SZ; }
    int    get_ivSize()    const { return DES_IV_SZ; }
    DES();
    ~DES();
private:
    struct DESImpl;
    DESImpl* pimpl_;

    DES(const DES&);                // hide copy
    DES& operator=(const DES&);     // & assign
};


// 3DES Encrypt-Decrypt-Encrypt in CBC mode
class DES_EDE : public BulkCipher {
public:
    void   encrypt(byte*, const byte*, unsigned int);
    void   decrypt(byte*, const byte*, unsigned int);
    void   set_encryptKey(const byte*, const byte*);
    void   set_decryptKey(const byte*, const byte*);
    uint   get_blockSize() const { return DES_BLOCK; }
    int    get_keySize()   const { return DES_EDE_KEY_SZ; }
    int    get_ivSize()    const { return DES_IV_SZ; }
    DES_EDE();
    ~DES_EDE();
private:
    struct DES_EDEImpl;
    DES_EDEImpl* pimpl_;

    DES_EDE(const DES_EDE&);            // hide copy
    DES_EDE& operator=(const DES_EDE&); // & assign
};


// Alledged RC4
class RC4 : public BulkCipher {
public:
    void encrypt(byte*, const byte*, unsigned int);
    void decrypt(byte*, const byte*, unsigned int);
    void set_encryptKey(const byte*, const byte*);
    void set_decryptKey(const byte*, const byte*);
    uint get_blockSize() const { return 0; }
    int  get_keySize()   const { return RC4_KEY_SZ; }
    int  get_ivSize()    const { return 0; }
    RC4();
    ~RC4();
private:
    struct RC4Impl;
    RC4Impl* pimpl_;

    RC4(const RC4&);             // hide copy
    RC4& operator=(const RC4&);  // & assign
};


// AES
class AES : public BulkCipher {
public:
    void encrypt(byte*, const byte*, unsigned int);
    void decrypt(byte*, const byte*, unsigned int);
    void set_encryptKey(const byte*, const byte*);
    void set_decryptKey(const byte*, const byte*);
    uint get_blockSize() const { return AES_BLOCK_SZ; }
    int  get_keySize()   const;
    int  get_ivSize()    const { return AES_IV_SZ; }
    explicit AES(unsigned int = AES_128_KEY_SZ);
    ~AES();
private:
    struct AESImpl;
    AESImpl* pimpl_;

    AES(const AES&);             // hide copy
    AES& operator=(const AES&);  // & assign
};


// Random number generator
class RandomPool {
public:
    void Fill(opaque* dst, uint sz) const;
    RandomPool();
    ~RandomPool();

    int GetError() const;

    friend class RSA;
    friend class DSS;
    friend class DiffieHellman;
private:
    struct RandomImpl;
    RandomImpl* pimpl_;

    RandomPool(const RandomPool&);              // hide copy
    RandomPool& operator=(const RandomPool&);   // & assign
};


// Authentication policy should implement sign, and verify
struct Auth : public virtual_base {
    virtual void sign(byte*, const byte*, unsigned int, const RandomPool&) = 0;
    virtual bool verify(const byte*, unsigned int, const byte*,
                        unsigned int) = 0;
    virtual uint get_signatureLength() const = 0;
    virtual ~Auth() {}
};


// For use with NULL Authentication schemes
struct NO_Auth : public Auth {
    void   sign(byte*, const byte*, unsigned int, const RandomPool&) {}
    bool   verify(const byte*, unsigned int, const byte*, unsigned int) 
                    { return true; }
};


// Digitial Signature Standard scheme
class DSS : public Auth {
public:
    void sign(byte*, const byte*, unsigned int, const RandomPool&);
    bool verify(const byte*, unsigned int, const byte*, unsigned int);
    uint get_signatureLength() const;
    DSS(const byte*, unsigned int, bool publicKey = true);
    ~DSS();
private:
    struct DSSImpl;
    DSSImpl* pimpl_;

    DSS(const DSS&);
    DSS& operator=(const DSS&);
};


// RSA Authentication and exchange
class RSA : public Auth {
public:
    void   sign(byte*, const byte*, unsigned int, const RandomPool&);
    bool   verify(const byte*, unsigned int, const byte*, unsigned int);
    void   encrypt(byte*, const byte*, unsigned int, const RandomPool&);
    void   decrypt(byte*, const byte*, unsigned int, const RandomPool&);
    uint   get_signatureLength() const;
    uint   get_cipherLength() const;
    RSA(const byte*, unsigned int, bool publicKey = true);
    ~RSA();
private:
    struct RSAImpl;
    RSAImpl* pimpl_;

    RSA(const RSA&);            // hide copy
    RSA& operator=(const RSA&); // & assing
};


class Integer;

// Diffie-Hellman agreement
// hide for now TODO: figure out a way to give access to C clients p and g args
class DiffieHellman  {
public:
    DiffieHellman(const byte*, unsigned int, const byte*, unsigned int,
                  const byte*, unsigned int, const RandomPool& random);
    //DiffieHellman(const char*, const RandomPool&);
    DiffieHellman(const Integer&, const Integer&, const RandomPool&);
    ~DiffieHellman();

    DiffieHellman(const DiffieHellman&);  
    DiffieHellman& operator=(const DiffieHellman&);

    uint        get_agreedKeyLength() const;
    const byte* get_agreedKey()       const;
    const byte* get_publicKey()       const;
    void        makeAgreement(const byte*, unsigned int);

    void        set_sizes(int&, int&, int&) const;
    void        get_parms(byte*, byte*, byte*) const;
private:
    struct DHImpl;
    DHImpl* pimpl_;
};


// Lagrge Integer
class Integer {
public:
    Integer();
    ~Integer();

    Integer(const Integer&);
    Integer& operator=(const Integer&);

    void assign(const byte*, unsigned int);

    friend class DiffieHellman;
private:
    struct IntegerImpl;
    IntegerImpl* pimpl_;
};


class x509;


struct EncryptedInfo {
    enum { IV_SZ = 32, NAME_SZ = 80 };
    char  name[NAME_SZ]; // max one line
    byte  iv[IV_SZ];     // in base16 rep
    uint  ivSz;
    bool  set;

    EncryptedInfo() : ivSz(0), set(false) {}
};

x509* PemToDer(FILE*, CertType, EncryptedInfo* info = 0);


} // naemspace

#endif  // yaSSL_CRYPTO_WRAPPER_HPP

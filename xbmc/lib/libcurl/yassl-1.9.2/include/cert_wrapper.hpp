/* cert_wrapper.hpp                          
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


/*  The certificate wrapper header defines certificate management functions
 *
 */


#ifndef yaSSL_CERT_WRAPPER_HPP
#define yaSSL_CERT_WRAPPER_HPP

#ifdef _MSC_VER
    // disable truncated debug symbols
    #pragma warning(disable:4786)
#endif


#include "yassl_types.hpp"  // SignatureAlgorithm
#include "buffer.hpp"       // input_buffer
#include "asn.hpp"          // SignerList
#include "openssl/ssl.h"    // internal and external use
#include STL_LIST_FILE
#include STL_ALGORITHM_FILE


namespace STL = STL_NAMESPACE;


namespace yaSSL {
   
typedef unsigned char opaque;
class X509;                     // forward openSSL type

using TaoCrypt::SignerList;

// an x509 version 3 certificate
class x509 {
    uint    length_;
    opaque* buffer_;
public:
    explicit x509(uint sz);
    ~x509();

    uint          get_length() const;
    const opaque* get_buffer() const;
    opaque*       use_buffer();

    x509(const x509&);
    x509& operator=(const x509&);
private:
    void Swap(x509&);
};


// Certificate Manager keeps a list of the cert chain and public key
class CertManager {
    typedef STL::list<x509*> CertList;

    CertList     list_;                 // self      
    input_buffer privateKey_;

    CertList     peerList_;             // peer
    input_buffer peerPublicKey_;
    X509*        peerX509_;             // peer's openSSL X509

    SignatureAlgorithm keyType_;        // self   key type
    SignatureAlgorithm peerKeyType_;    // peer's key type

    SignerList   signers_;              // decoded CA keys and names
                                        //    plus verified chained certs
    bool verifyPeer_;
    bool verifyNone_;                   // no error if verify fails
    bool failNoCert_;
    bool sendVerify_;
    VerifyCallback verifyCallback_;     // user verify callback
public:
    CertManager();
    ~CertManager();

    void AddPeerCert(x509* x);      // take ownership
    void CopySelfCert(const x509* x);
    int  CopyCaCert(const x509* x);
    int  Validate();

    int SetPrivateKey(const x509&);

    const x509*        get_cert()        const;
    const opaque*      get_peerKey()     const;
    const opaque*      get_privateKey()  const;
          X509*        get_peerX509()    const;
    SignatureAlgorithm get_keyType()     const;
    SignatureAlgorithm get_peerKeyType() const;

    uint get_peerKeyLength()       const;
    uint get_privateKeyLength()    const;

    bool verifyPeer() const;
    bool verifyNone() const;
    bool failNoCert() const;
    bool sendVerify() const;

    void setVerifyPeer();
    void setVerifyNone();
    void setFailNoCert();
    void setSendVerify();
    void setPeerX509(X509*);
    void setVerifyCallback(VerifyCallback);
private:
    CertManager(const CertManager&);            // hide copy
    CertManager& operator=(const CertManager&); // and assign
};


} // naemspace

#endif // yaSSL_CERT_WRAPPER_HPP

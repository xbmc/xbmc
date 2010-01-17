/* cert_wrapper.cpp                          
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


/*  The certificate wrapper source implements certificate management functions
 *
 */

#include "runtime.hpp"
#include "cert_wrapper.hpp"
#include "yassl_int.hpp"
#include "error.hpp"

#if defined(USE_CML_LIB)
    #include "cmapi_cpp.h"
#else
    #include "asn.hpp"
    #include "file.hpp"
#endif // USE_CML_LIB


namespace yaSSL {


x509::x509(uint sz) : length_(sz), buffer_(NEW_YS opaque[sz]) 
{
}


x509::~x509() 
{ 
    ysArrayDelete(buffer_); 
}


x509::x509(const x509& that) : length_(that.length_),
                               buffer_(NEW_YS opaque[length_])
{
    memcpy(buffer_, that.buffer_, length_);
}


void x509::Swap(x509& that)
{
    STL::swap(length_, that.length_);
    STL::swap(buffer_, that.buffer_);
}


x509& x509::operator=(const x509& that)
{
    x509 temp(that);
    Swap(temp);
    return *this;
}


uint x509::get_length() const
{ 
    return length_; 
}


const opaque* x509::get_buffer() const
{ 
    return buffer_; 
}


opaque* x509::use_buffer()
{ 
    return buffer_; 
}


//CertManager
CertManager::CertManager()
    : peerX509_(0), verifyPeer_(false), verifyNone_(false), failNoCert_(false),
      sendVerify_(false), verifyCallback_(0)
{}


CertManager::~CertManager()
{
    ysDelete(peerX509_);

    STL::for_each(signers_.begin(), signers_.end(), del_ptr_zero()) ;

    STL::for_each(peerList_.begin(), peerList_.end(), del_ptr_zero()) ;

    STL::for_each(list_.begin(), list_.end(), del_ptr_zero()) ;
}


bool CertManager::verifyPeer() const
{
    return verifyPeer_;
}


bool CertManager::verifyNone() const
{
    return verifyNone_;
}


bool CertManager::failNoCert() const
{
    return failNoCert_;
}


bool CertManager::sendVerify() const
{
    return sendVerify_;
}


void CertManager::setVerifyPeer()
{
    verifyPeer_ = true;
}


void CertManager::setVerifyNone()
{
    verifyNone_ = true;
}


void CertManager::setFailNoCert()
{
    failNoCert_ = true;
}


void CertManager::setSendVerify()
{
    sendVerify_ = true;
}


void CertManager::setVerifyCallback(VerifyCallback vc)
{
    verifyCallback_ = vc;
}


void CertManager::AddPeerCert(x509* x)
{ 
    peerList_.push_back(x);  // take ownership
}


void CertManager::CopySelfCert(const x509* x)
{
    if (x)
        list_.push_back(NEW_YS x509(*x));
}


// add to signers
int CertManager::CopyCaCert(const x509* x)
{
    TaoCrypt::Source source(x->get_buffer(), x->get_length());
    TaoCrypt::CertDecoder cert(source, true, &signers_, verifyNone_,
                               TaoCrypt::CertDecoder::CA);

    if (!cert.GetError().What()) {
        const TaoCrypt::PublicKey& key = cert.GetPublicKey();
        signers_.push_back(NEW_YS TaoCrypt::Signer(key.GetKey(), key.size(),
                                        cert.GetCommonName(), cert.GetHash()));
    }
    // just don't add, not an error return cert.GetError().What();
    return 0;
}


const x509* CertManager::get_cert() const
{ 
    return list_.front();
}


const opaque* CertManager::get_peerKey() const
{ 
    return peerPublicKey_.get_buffer();
}


X509* CertManager::get_peerX509() const
{
    return peerX509_;
}


SignatureAlgorithm CertManager::get_peerKeyType() const
{
    return peerKeyType_;
}


SignatureAlgorithm CertManager::get_keyType() const
{
    return keyType_;
}


uint CertManager::get_peerKeyLength() const
{ 
    return peerPublicKey_.get_size();
}


const opaque* CertManager::get_privateKey() const
{ 
    return privateKey_.get_buffer();
}


uint CertManager::get_privateKeyLength() const
{ 
    return privateKey_.get_size();
}


// Validate the peer's certificate list, from root to peer (last to first)
int CertManager::Validate()
{
    CertList::reverse_iterator last = peerList_.rbegin();
    size_t count = peerList_.size();

    while ( count > 1 ) {
        TaoCrypt::Source source((*last)->get_buffer(), (*last)->get_length());
        TaoCrypt::CertDecoder cert(source, true, &signers_, verifyNone_);

        if (int err = cert.GetError().What())
            return err;

        const TaoCrypt::PublicKey& key = cert.GetPublicKey();
        signers_.push_back(NEW_YS TaoCrypt::Signer(key.GetKey(), key.size(),
                                        cert.GetCommonName(), cert.GetHash()));
        ++last;
        --count;
    }

    if (count) {
        // peer's is at the front
        TaoCrypt::Source source((*last)->get_buffer(), (*last)->get_length());
        TaoCrypt::CertDecoder cert(source, true, &signers_, verifyNone_);

        int err = cert.GetError().What();
        if ( err && err != TaoCrypt::SIG_OTHER_E)
            return err;

        uint sz = cert.GetPublicKey().size();
        peerPublicKey_.allocate(sz);
        peerPublicKey_.assign(cert.GetPublicKey().GetKey(), sz);

        if (cert.GetKeyType() == TaoCrypt::RSAk)
            peerKeyType_ = rsa_sa_algo;
        else
            peerKeyType_ = dsa_sa_algo;

        size_t iSz = strlen(cert.GetIssuer()) + 1;
        size_t sSz = strlen(cert.GetCommonName()) + 1;
        int bSz = (int)strlen(cert.GetBeforeDate()) + 1;
        int aSz = (int)strlen(cert.GetAfterDate()) + 1;
        peerX509_ = NEW_YS X509(cert.GetIssuer(), iSz, cert.GetCommonName(),
                                sSz, cert.GetBeforeDate(), bSz,
                                cert.GetAfterDate(), aSz);

        if (err == TaoCrypt::SIG_OTHER_E && verifyCallback_) {
            X509_STORE_CTX store;
            store.error = err;
            store.error_depth = static_cast<int>(count) - 1;
            store.current_cert = peerX509_;

            int ok = verifyCallback_(0, &store);
            if (ok) return 0;
        }

        if (err == TaoCrypt::SIG_OTHER_E) return err;
    }
    return 0;
}


// Set the private key
int CertManager::SetPrivateKey(const x509& key)
{
    privateKey_.allocate(key.get_length());
    privateKey_.assign(key.get_buffer(), key.get_length());

    // set key type
    if (x509* cert = list_.front()) {
        TaoCrypt::Source source(cert->get_buffer(), cert->get_length());
        TaoCrypt::CertDecoder cd(source, false);
        cd.DecodeToKey();
        if (int err = cd.GetError().What())
            return err;
        if (cd.GetKeyType() == TaoCrypt::RSAk)
            keyType_ = rsa_sa_algo;
        else
            keyType_ = dsa_sa_algo;
    }
    return 0;
}


// Store OpenSSL type peer's cert
void CertManager::setPeerX509(X509* x)
{
    assert(peerX509_ == 0);
    if (x == 0) return;

    X509_NAME* issuer   = x->GetIssuer();
    X509_NAME* subject  = x->GetSubject();
    ASN1_STRING* before = x->GetBefore();
    ASN1_STRING* after  = x->GetAfter();

    peerX509_ = NEW_YS X509(issuer->GetName(), issuer->GetLength(),
        subject->GetName(), subject->GetLength(), (const char*) before->data,
        before->length, (const char*) after->data, after->length);
}


#if defined(USE_CML_LIB)

// Get the peer's certificate, extract and save public key
void CertManager::SetPeerKey()
{
    // first cert is the peer's
    x509* main = peerList_.front();

    Bytes_struct cert;
    cert.num  = main->get_length();
    cert.data = main->set_buffer();

    CML::Certificate cm(cert);
    const CML::ASN::Cert& raw = cm.base();
    CTIL::CSM_Buffer key = raw.pubKeyInfo.key;

    uint sz;
    opaque* key_buffer = reinterpret_cast<opaque*>(key.Get(sz));
    peerPublicKey_.allocate(sz);
    peerPublicKey_.assign(key_buffer, sz);
}


#endif // USE_CML_LIB



} // namespace

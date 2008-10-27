/* yassl_imp.cpp                                
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

/*  yaSSL source implements all SSL.v3 secification structures.
 */

#include "runtime.hpp"
#include "yassl_int.hpp"
#include "handshake.hpp"

#include "asn.hpp"  // provide crypto wrapper??



namespace yaSSL {


namespace { // locals

bool isTLS(ProtocolVersion pv)
{
    if (pv.major_ >= 3 && pv.minor_ >= 1)
        return true;

    return false;
}


}  // namespace (locals)


void hashHandShake(SSL&, const input_buffer&, uint);


ProtocolVersion::ProtocolVersion(uint8 maj, uint8 min) 
    : major_(maj), minor_(min) 
{}


// construct key exchange with known ssl parms
void ClientKeyExchange::createKey(SSL& ssl)
{
    const ClientKeyFactory& ckf = ssl.getFactory().getClientKey();
    client_key_ = ckf.CreateObject(ssl.getSecurity().get_parms().kea_);

    if (!client_key_)
        ssl.SetError(factory_error);
}


// construct key exchange with known ssl parms
void ServerKeyExchange::createKey(SSL& ssl)
{
    const ServerKeyFactory& skf = ssl.getFactory().getServerKey();
    server_key_ = skf.CreateObject(ssl.getSecurity().get_parms().kea_);

    if (!server_key_)
        ssl.SetError(factory_error);
}


// build/set PreMaster secret and encrypt, client side
void EncryptedPreMasterSecret::build(SSL& ssl)
{
    opaque tmp[SECRET_LEN];
    memset(tmp, 0, sizeof(tmp));
    ssl.getCrypto().get_random().Fill(tmp, SECRET_LEN);
    ProtocolVersion pv = ssl.getSecurity().get_connection().chVersion_;
    tmp[0] = pv.major_;
    tmp[1] = pv.minor_;
    ssl.set_preMaster(tmp, SECRET_LEN);

    const CertManager& cert = ssl.getCrypto().get_certManager();
    RSA rsa(cert.get_peerKey(), cert.get_peerKeyLength());
    bool tls = ssl.isTLS();     // if TLS, put length for encrypted data
    alloc(rsa.get_cipherLength() + (tls ? 2 : 0));
    byte* holder = secret_;
    if (tls) {
        byte len[2];
        c16toa(rsa.get_cipherLength(), len);
        memcpy(secret_, len, sizeof(len));
        holder += 2;
    }
    rsa.encrypt(holder, tmp, SECRET_LEN, ssl.getCrypto().get_random());
}


// build/set premaster and Client Public key, client side
void ClientDiffieHellmanPublic::build(SSL& ssl)
{
    DiffieHellman& dhServer = ssl.useCrypto().use_dh();
    DiffieHellman  dhClient(dhServer);

    uint keyLength = dhClient.get_agreedKeyLength(); // pub and agree same

    alloc(keyLength, true);
    dhClient.makeAgreement(dhServer.get_publicKey(), keyLength);
    c16toa(keyLength, Yc_);
    memcpy(Yc_ + KEY_OFFSET, dhClient.get_publicKey(), keyLength);

    // because of encoding first byte might be zero, don't use it for preMaster
    if (*dhClient.get_agreedKey() == 0) 
        ssl.set_preMaster(dhClient.get_agreedKey() + 1, keyLength - 1);
    else
        ssl.set_preMaster(dhClient.get_agreedKey(), keyLength);
}


// build server exhange, server side
void DH_Server::build(SSL& ssl)
{
    DiffieHellman& dhServer = ssl.useCrypto().use_dh();

    int pSz, gSz, pubSz;
    dhServer.set_sizes(pSz, gSz, pubSz);
    dhServer.get_parms(parms_.alloc_p(pSz), parms_.alloc_g(gSz),
                       parms_.alloc_pub(pubSz));

    short sigSz = 0;
    mySTL::auto_ptr<Auth> auth;
    const CertManager& cert = ssl.getCrypto().get_certManager();
    
    if (ssl.getSecurity().get_parms().sig_algo_ == rsa_sa_algo)
        auth.reset(NEW_YS RSA(cert.get_privateKey(),
                   cert.get_privateKeyLength(), false));
    else {
        auth.reset(NEW_YS DSS(cert.get_privateKey(),
                   cert.get_privateKeyLength(), false));
        sigSz += DSS_ENCODED_EXTRA;
    }
    
    sigSz += auth->get_signatureLength();
    if (!sigSz) {
        ssl.SetError(privateKey_error);
        return;
    }

    length_ = 8; // pLen + gLen + YsLen + SigLen
    length_ += pSz + gSz + pubSz + sigSz;

    output_buffer tmp(length_);
    byte len[2];
    // P
    c16toa(pSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(parms_.get_p(), pSz);
    // G
    c16toa(gSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(parms_.get_g(), gSz);
    // Ys
    c16toa(pubSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(parms_.get_pub(), pubSz);

    // Sig
    byte hash[FINISHED_SZ];
    MD5  md5;
    SHA  sha;
    signature_ = NEW_YS byte[sigSz];

    const Connection& conn = ssl.getSecurity().get_connection();
    // md5
    md5.update(conn.client_random_, RAN_LEN);
    md5.update(conn.server_random_, RAN_LEN);
    md5.update(tmp.get_buffer(), tmp.get_size());
    md5.get_digest(hash);

    // sha
    sha.update(conn.client_random_, RAN_LEN);
    sha.update(conn.server_random_, RAN_LEN);
    sha.update(tmp.get_buffer(), tmp.get_size());
    sha.get_digest(&hash[MD5_LEN]);

    if (ssl.getSecurity().get_parms().sig_algo_ == rsa_sa_algo)
        auth->sign(signature_, hash, sizeof(hash),
                   ssl.getCrypto().get_random());
    else {
        auth->sign(signature_, &hash[MD5_LEN], SHA_LEN,
                   ssl.getCrypto().get_random());
        byte encoded[DSS_SIG_SZ + DSS_ENCODED_EXTRA];
        TaoCrypt::EncodeDSA_Signature(signature_, encoded);
        memcpy(signature_, encoded, sizeof(encoded));
    }

    c16toa(sigSz, len);
    tmp.write(len, sizeof(len));
    tmp.write(signature_, sigSz);

    // key message
    keyMessage_ = NEW_YS opaque[length_];
    memcpy(keyMessage_, tmp.get_buffer(), tmp.get_size());
}


// read PreMaster secret and decrypt, server side
void EncryptedPreMasterSecret::read(SSL& ssl, input_buffer& input)
{
    const CertManager& cert = ssl.getCrypto().get_certManager();
    RSA rsa(cert.get_privateKey(), cert.get_privateKeyLength(), false);
    uint16 cipherLen = rsa.get_cipherLength();
    if (ssl.isTLS()) {
        byte len[2];
        input.read(len, sizeof(len));
        ato16(len, cipherLen);
    }
    alloc(cipherLen);
    input.read(secret_, length_);

    opaque preMasterSecret[SECRET_LEN];
    rsa.decrypt(preMasterSecret, secret_, length_, 
                ssl.getCrypto().get_random());

    ProtocolVersion pv = ssl.getSecurity().get_connection().chVersion_;
    if (pv.major_ != preMasterSecret[0] || pv.minor_ != preMasterSecret[1])
        ssl.SetError(pms_version_error); // continue deriving for timing attack

    ssl.set_preMaster(preMasterSecret, SECRET_LEN);
    ssl.makeMasterSecret();
}


EncryptedPreMasterSecret::EncryptedPreMasterSecret()
    : secret_(0), length_(0)
{}


EncryptedPreMasterSecret::~EncryptedPreMasterSecret()
{
    ysArrayDelete(secret_);
}


int EncryptedPreMasterSecret::get_length() const
{
    return length_;
}


opaque* EncryptedPreMasterSecret::get_clientKey() const
{
    return secret_;
}


void EncryptedPreMasterSecret::alloc(int sz)
{
    length_ = sz;
    secret_ = NEW_YS opaque[sz];
}


// read client's public key, server side
void ClientDiffieHellmanPublic::read(SSL& ssl, input_buffer& input)
{
    DiffieHellman& dh = ssl.useCrypto().use_dh();

    uint16 keyLength;
    byte tmp[2];
    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    ato16(tmp, keyLength);

    alloc(keyLength);
    input.read(Yc_, keyLength);
    dh.makeAgreement(Yc_, keyLength); 

    // because of encoding, first byte might be 0, don't use for preMaster 
    if (*dh.get_agreedKey() == 0) 
        ssl.set_preMaster(dh.get_agreedKey() + 1, dh.get_agreedKeyLength() - 1);
    else
        ssl.set_preMaster(dh.get_agreedKey(), dh.get_agreedKeyLength());
    ssl.makeMasterSecret();
}


ClientDiffieHellmanPublic::ClientDiffieHellmanPublic()
    : length_(0), Yc_(0)
{}


ClientDiffieHellmanPublic::~ClientDiffieHellmanPublic()
{
    ysArrayDelete(Yc_);
}


int ClientDiffieHellmanPublic::get_length() const
{
    return length_;
}


opaque* ClientDiffieHellmanPublic::get_clientKey() const
{
    return Yc_;
}


void ClientDiffieHellmanPublic::alloc(int sz, bool offset) 
{
    length_ = sz + (offset ? KEY_OFFSET : 0); 
    Yc_ = NEW_YS opaque[length_];
}


// read server's p, g, public key and sig, client side
void DH_Server::read(SSL& ssl, input_buffer& input)
{
    uint16 length, messageTotal = 6; // pSz + gSz + pubSz
    byte tmp[2];

    // p
    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    ato16(tmp, length);
    messageTotal += length;

    input.read(parms_.alloc_p(length), length);

    // g
    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    ato16(tmp, length);
    messageTotal += length;

    input.read(parms_.alloc_g(length), length);

    // pub
    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    ato16(tmp, length);
    messageTotal += length;

    input.read(parms_.alloc_pub(length), length);

    // save message for hash verify
    input_buffer message(messageTotal);
    input.set_current(input.get_current() - messageTotal);
    input.read(message.get_buffer(), messageTotal);
    message.add_size(messageTotal);

    // signature
    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    ato16(tmp, length);

    signature_ = NEW_YS byte[length];
    input.read(signature_, length);

    // verify signature
    byte hash[FINISHED_SZ];
    MD5  md5;
    SHA  sha;

    const Connection& conn = ssl.getSecurity().get_connection();
    // md5
    md5.update(conn.client_random_, RAN_LEN);
    md5.update(conn.server_random_, RAN_LEN);
    md5.update(message.get_buffer(), message.get_size());
    md5.get_digest(hash);

    // sha
    sha.update(conn.client_random_, RAN_LEN);
    sha.update(conn.server_random_, RAN_LEN);
    sha.update(message.get_buffer(), message.get_size());
    sha.get_digest(&hash[MD5_LEN]);

    const CertManager& cert = ssl.getCrypto().get_certManager();
    
    if (ssl.getSecurity().get_parms().sig_algo_ == rsa_sa_algo) {
        RSA rsa(cert.get_peerKey(), cert.get_peerKeyLength());
        if (!rsa.verify(hash, sizeof(hash), signature_, length))
            ssl.SetError(verify_error);
    }
    else {
        byte decodedSig[DSS_SIG_SZ];
        length = TaoCrypt::DecodeDSA_Signature(decodedSig, signature_, length);
        
        DSS dss(cert.get_peerKey(), cert.get_peerKeyLength());
        if (!dss.verify(&hash[MD5_LEN], SHA_LEN, decodedSig, length))
            ssl.SetError(verify_error);
    }

    // save input
    ssl.useCrypto().SetDH(NEW_YS DiffieHellman(parms_.get_p(),
               parms_.get_pSize(), parms_.get_g(), parms_.get_gSize(),
               parms_.get_pub(), parms_.get_pubSize(),
               ssl.getCrypto().get_random()));
}


DH_Server::DH_Server()
    : signature_(0), length_(0), keyMessage_(0)
{}


DH_Server::~DH_Server()
{
    ysArrayDelete(keyMessage_);
    ysArrayDelete(signature_);
}


int DH_Server::get_length() const
{
    return length_;
}


opaque* DH_Server::get_serverKey() const
{
    return keyMessage_;
}


// set available suites
Parameters::Parameters(ConnectionEnd ce, const Ciphers& ciphers, 
                       ProtocolVersion pv, bool haveDH) : entity_(ce)
{
    pending_ = true;	// suite not set yet
    strncpy(cipher_name_, "NONE", 5);

    if (ciphers.setSuites_) {   // use user set list
        suites_size_ = ciphers.suiteSz_;
        memcpy(suites_, ciphers.suites_, ciphers.suiteSz_);
        SetCipherNames();
    }
    else 
        SetSuites(pv, ce == server_end && !haveDH);  // defaults

}


void Parameters::SetSuites(ProtocolVersion pv, bool removeDH)
{
    int i = 0;
    // available suites, best first
    // when adding more, make sure cipher_names is updated and
    //      MAX_CIPHERS is big enough

    if (isTLS(pv)) {
        if (!removeDH) {
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_RSA_WITH_AES_256_CBC_SHA;
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_DSS_WITH_AES_256_CBC_SHA;
        }
        suites_[i++] = 0x00;
        suites_[i++] = TLS_RSA_WITH_AES_256_CBC_SHA;

        if (!removeDH) {
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_RSA_WITH_AES_128_CBC_SHA;
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_DSS_WITH_AES_128_CBC_SHA;
        }
        suites_[i++] = 0x00;
        suites_[i++] = TLS_RSA_WITH_AES_128_CBC_SHA;

        suites_[i++] = 0x00;
        suites_[i++] = TLS_RSA_WITH_AES_256_CBC_RMD160;
        suites_[i++] = 0x00;
        suites_[i++] = TLS_RSA_WITH_AES_128_CBC_RMD160;
        suites_[i++] = 0x00;
        suites_[i++] = TLS_RSA_WITH_3DES_EDE_CBC_RMD160;

        if (!removeDH) {
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_RSA_WITH_AES_256_CBC_RMD160;
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_RSA_WITH_AES_128_CBC_RMD160;
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_RSA_WITH_3DES_EDE_CBC_RMD160;

            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_DSS_WITH_AES_256_CBC_RMD160;
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_DSS_WITH_AES_128_CBC_RMD160;
            suites_[i++] = 0x00;
            suites_[i++] = TLS_DHE_DSS_WITH_3DES_EDE_CBC_RMD160;
        }
    }

    suites_[i++] = 0x00;
    suites_[i++] = SSL_RSA_WITH_RC4_128_SHA;  
    suites_[i++] = 0x00;
    suites_[i++] = SSL_RSA_WITH_RC4_128_MD5;

    suites_[i++] = 0x00;
    suites_[i++] = SSL_RSA_WITH_3DES_EDE_CBC_SHA;
    suites_[i++] = 0x00;
    suites_[i++] = SSL_RSA_WITH_DES_CBC_SHA;

    if (!removeDH) {
        suites_[i++] = 0x00;
        suites_[i++] = SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA;  
        suites_[i++] = 0x00;
        suites_[i++] = SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA; 

        suites_[i++] = 0x00;
        suites_[i++] = SSL_DHE_RSA_WITH_DES_CBC_SHA;  
        suites_[i++] = 0x00;
        suites_[i++] = SSL_DHE_DSS_WITH_DES_CBC_SHA;
    }

    suites_size_ = i;

    SetCipherNames();
}


void Parameters::SetCipherNames()
{
    const int suites = suites_size_ / 2;
    int pos = 0;

    for (int j = 0; j < suites; j++) {
        int index = suites_[j*2 + 1];  // every other suite is suite id
        size_t len = strlen(cipher_names[index]) + 1;
        strncpy(cipher_list_[pos++], cipher_names[index], len);
    }
    cipher_list_[pos][0] = 0;
}


// input operator for RecordLayerHeader, adjust stream
input_buffer& operator>>(input_buffer& input, RecordLayerHeader& hdr)
{
    hdr.type_ = ContentType(input[AUTO]);
    hdr.version_.major_ = input[AUTO];
    hdr.version_.minor_ = input[AUTO];

    // length
    byte tmp[2];
    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    ato16(tmp, hdr.length_);

    return input;
}


// output operator for RecordLayerHeader
output_buffer& operator<<(output_buffer& output, const RecordLayerHeader& hdr)
{
    output[AUTO] = hdr.type_;
    output[AUTO] = hdr.version_.major_;
    output[AUTO] = hdr.version_.minor_;
    
    // length
    byte tmp[2];
    c16toa(hdr.length_, tmp);
    output[AUTO] = tmp[0];
    output[AUTO] = tmp[1];

    return output;
}


// virtual input operator for Messages
input_buffer& operator>>(input_buffer& input, Message& msg)
{
    return msg.set(input);
}

// virtual output operator for Messages
output_buffer& operator<<(output_buffer& output, const Message& msg)
{
    return msg.get(output);
}


// input operator for HandShakeHeader
input_buffer& operator>>(input_buffer& input, HandShakeHeader& hs)
{
    hs.type_ = HandShakeType(input[AUTO]);

    hs.length_[0] = input[AUTO];
    hs.length_[1] = input[AUTO];
    hs.length_[2] = input[AUTO];
    
    return input;
}


// output operator for HandShakeHeader
output_buffer& operator<<(output_buffer& output, const HandShakeHeader& hdr)
{
    output[AUTO] = hdr.type_;
    output.write(hdr.length_, sizeof(hdr.length_));
    return output;
}


// HandShake Header Processing function
void HandShakeHeader::Process(input_buffer& input, SSL& ssl)
{
    ssl.verifyState(*this);
    if (ssl.GetError()) return;
    const HandShakeFactory& hsf = ssl.getFactory().getHandShake();
    mySTL::auto_ptr<HandShakeBase> hs(hsf.CreateObject(type_));
    if (!hs.get()) {
        ssl.SetError(factory_error);
        return;
    }

    uint len = c24to32(length_);
    if (len > input.get_remaining()) {
        ssl.SetError(bad_input);
        return;
    }
    hashHandShake(ssl, input, len);

    hs->set_length(len);
    input >> *hs;
    hs->Process(input, ssl);
}


ContentType HandShakeHeader::get_type() const
{
    return handshake;
}


uint16 HandShakeHeader::get_length() const
{
    return c24to32(length_);
}


HandShakeType HandShakeHeader::get_handshakeType() const
{
    return type_;
}


void HandShakeHeader::set_type(HandShakeType hst)
{
    type_ = hst;
}


void HandShakeHeader::set_length(uint32 u32)
{
    c32to24(u32, length_);
}


input_buffer& HandShakeHeader::set(input_buffer& in)
{
    return in >> *this;
}


output_buffer& HandShakeHeader::get(output_buffer& out) const
{
    return out << *this;
}



int HandShakeBase::get_length() const
{
    return length_;
}


void HandShakeBase::set_length(int l)
{
    length_ = l;
}


// for building buffer's type field
HandShakeType HandShakeBase::get_type() const
{
    return no_shake;
}


input_buffer& HandShakeBase::set(input_buffer& in)
{
    return in;
}

 
output_buffer& HandShakeBase::get(output_buffer& out) const
{
    return out;
}


void HandShakeBase::Process(input_buffer&, SSL&) 
{}


input_buffer& HelloRequest::set(input_buffer& in)
{
    return in;
}


output_buffer& HelloRequest::get(output_buffer& out) const
{
    return out;
}


void HelloRequest::Process(input_buffer&, SSL&)
{}


HandShakeType HelloRequest::get_type() const
{
    return hello_request;
}


// input operator for CipherSpec
input_buffer& operator>>(input_buffer& input, ChangeCipherSpec& cs)
{
    cs.type_ = CipherChoice(input[AUTO]);
    return input; 
}

// output operator for CipherSpec
output_buffer& operator<<(output_buffer& output, const ChangeCipherSpec& cs)
{
    output[AUTO] = cs.type_;
    return output;
}


ChangeCipherSpec::ChangeCipherSpec() 
    : type_(change_cipher_spec_choice)
{}


input_buffer& ChangeCipherSpec::set(input_buffer& in)
{
    return in >> *this;
}


output_buffer& ChangeCipherSpec::get(output_buffer& out) const
{
    return out << *this;
}


ContentType ChangeCipherSpec::get_type() const
{
    return change_cipher_spec;
}


uint16 ChangeCipherSpec::get_length() const
{
    return SIZEOF_ENUM;
}


// CipherSpec processing handler
void ChangeCipherSpec::Process(input_buffer&, SSL& ssl)
{
    ssl.useSecurity().use_parms().pending_ = false;
    if (ssl.getSecurity().get_resuming()) {
        if (ssl.getSecurity().get_parms().entity_ == client_end)
            buildFinished(ssl, ssl.useHashes().use_verify(), server); // server
    }
    else if (ssl.getSecurity().get_parms().entity_ == server_end)
        buildFinished(ssl, ssl.useHashes().use_verify(), client);     // client
}


Alert::Alert(AlertLevel al, AlertDescription ad)
    : level_(al), description_(ad)
{}


ContentType Alert::get_type() const
{
    return alert;
}


uint16 Alert::get_length() const
{
    return SIZEOF_ENUM * 2;
}


input_buffer& Alert::set(input_buffer& in)
{
    return in >> *this;
}


output_buffer& Alert::get(output_buffer& out) const
{
    return out << *this;
}


// input operator for Alert
input_buffer& operator>>(input_buffer& input, Alert& a)
{
    a.level_ = AlertLevel(input[AUTO]);
    a.description_ = AlertDescription(input[AUTO]);
 
    return input;
}


// output operator for Alert
output_buffer& operator<<(output_buffer& output, const Alert& a)
{
    output[AUTO] = a.level_;
    output[AUTO] = a.description_;
    return output;
}


// Alert processing handler
void Alert::Process(input_buffer& input, SSL& ssl)
{
    if (ssl.getSecurity().get_parms().pending_ == false)  { // encrypted alert
        int            aSz = get_length();  // alert size already read on input
        opaque         verify[SHA_LEN];
        const  opaque* data = input.get_buffer() + input.get_current() - aSz;

        if (ssl.isTLS())
            TLS_hmac(ssl, verify, data, aSz, alert, true);
        else
            hmac(ssl, verify, data, aSz, alert, true);

        // read mac and fill
        int    digestSz = ssl.getCrypto().get_digest().get_digestSize();
        opaque mac[SHA_LEN];
        input.read(mac, digestSz);

        if (ssl.getSecurity().get_parms().cipher_type_ == block) {
            int    ivExtra = 0;
            opaque fill;

            if (ssl.isTLSv1_1())
                ivExtra = ssl.getCrypto().get_cipher().get_blockSize();
            int padSz = ssl.getSecurity().get_parms().encrypt_size_ - ivExtra -
                        aSz - digestSz;
            for (int i = 0; i < padSz; i++) 
                fill = input[AUTO];
        }

        // verify
        if (memcmp(mac, verify, digestSz)) {
            ssl.SetError(verify_error);
            return;
        }
    }
    if (level_ == fatal) {
        ssl.useStates().useRecord()    = recordNotReady;
        ssl.useStates().useHandShake() = handShakeNotReady;
        ssl.SetError(YasslError(description_));
    }
}


Data::Data()
    : length_(0), buffer_(0), write_buffer_(0)
{}


Data::Data(uint16 len, opaque* b)
    : length_(len), buffer_(b), write_buffer_(0)
{}


void Data::SetData(uint16 len, const opaque* buffer)
{
    assert(write_buffer_ == 0);

    length_ = len;
    write_buffer_ = buffer;
}

input_buffer& Data::set(input_buffer& in)
{
    return in;
}


output_buffer& Data::get(output_buffer& out) const
{
    return out << *this;
}


ContentType Data::get_type() const
{
    return application_data;
}


uint16 Data::get_length() const
{
    return length_;
}


void Data::set_length(uint16 l)
{
    length_ = l;
}


opaque* Data::set_buffer()
{
    return buffer_;
}


// output operator for Data
output_buffer& operator<<(output_buffer& output, const Data& data)
{
    output.write(data.write_buffer_, data.length_);
    return output;
}


// Process handler for Data
void Data::Process(input_buffer& input, SSL& ssl)
{
    int msgSz = ssl.getSecurity().get_parms().encrypt_size_;
    int pad   = 0, padByte = 0;
    int ivExtra = 0;

    if (ssl.getSecurity().get_parms().cipher_type_ == block) {
        if (ssl.isTLSv1_1())  // IV
            ivExtra = ssl.getCrypto().get_cipher().get_blockSize();
        pad = *(input.get_buffer() + input.get_current() + msgSz -ivExtra - 1);
        padByte = 1;
    }
    int digestSz = ssl.getCrypto().get_digest().get_digestSize();
    int dataSz = msgSz - ivExtra - digestSz - pad - padByte;   
    opaque verify[SHA_LEN];

    const byte* rawData = input.get_buffer() + input.get_current();

    // read data
    if (dataSz) {                               // could be compressed
        if (ssl.CompressionOn()) {
            input_buffer tmp;
            if (DeCompress(input, dataSz, tmp) == -1) {
                ssl.SetError(decompress_error);
                return;
            }
            ssl.addData(NEW_YS input_buffer(tmp.get_size(),
                                            tmp.get_buffer(), tmp.get_size()));
        }
        else {
            input_buffer* data;
            ssl.addData(data = NEW_YS input_buffer(dataSz));
            input.read(data->get_buffer(), dataSz);
            data->add_size(dataSz);
        }

        if (ssl.isTLS())
            TLS_hmac(ssl, verify, rawData, dataSz, application_data, true);
        else
            hmac(ssl, verify, rawData, dataSz, application_data, true);
    }

    // read mac and fill
    opaque mac[SHA_LEN];
    opaque fill;
    input.read(mac, digestSz);
    for (int i = 0; i < pad; i++) 
        fill = input[AUTO];
    if (padByte)
        fill = input[AUTO];    

    // verify
    if (dataSz) {
        if (memcmp(mac, verify, digestSz)) {
            ssl.SetError(verify_error);
            return;
        }
    }
    else 
        ssl.get_SEQIncrement(true);  // even though no data, increment verify
}


// virtual input operator for HandShakes
input_buffer& operator>>(input_buffer& input, HandShakeBase& hs)
{
    return hs.set(input);
}


// virtual output operator for HandShakes
output_buffer& operator<<(output_buffer& output, const HandShakeBase& hs)
{
    return hs.get(output);
}


Certificate::Certificate(const x509* cert) : cert_(cert) 
{
    set_length(cert_->get_length() + 2 * CERT_HEADER); // list and cert size
}


const opaque* Certificate::get_buffer() const
{
    return cert_->get_buffer(); 
}


// output operator for Certificate
output_buffer& operator<<(output_buffer& output, const Certificate& cert)
{
    uint sz = cert.get_length() - 2 * CERT_HEADER;
    opaque tmp[CERT_HEADER];

    c32to24(sz + CERT_HEADER, tmp);
    output.write(tmp, CERT_HEADER);
    c32to24(sz, tmp);
    output.write(tmp, CERT_HEADER);
    output.write(cert.get_buffer(), sz);

    return output;
}


// certificate processing handler
void Certificate::Process(input_buffer& input, SSL& ssl)
{
    CertManager& cm = ssl.useCrypto().use_certManager();
  
    uint32 list_sz;
    byte   tmp[3];

    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    tmp[2] = input[AUTO];
    c24to32(tmp, list_sz);
    
    while (list_sz) {
        // cert size
        uint32 cert_sz;
        tmp[0] = input[AUTO];
        tmp[1] = input[AUTO];
        tmp[2] = input[AUTO];
        c24to32(tmp, cert_sz);
        
        x509* myCert;
        cm.AddPeerCert(myCert = NEW_YS x509(cert_sz));
        input.read(myCert->use_buffer(), myCert->get_length());

        list_sz -= cert_sz + CERT_HEADER;
    }
    if (int err = cm.Validate())
        ssl.SetError(YasslError(err));
    else if (ssl.getSecurity().get_parms().entity_ == client_end)
        ssl.useStates().useClient() = serverCertComplete;
}


Certificate::Certificate()
    : cert_(0)
{}


input_buffer& Certificate::set(input_buffer& in)
{
    return in;
}


output_buffer& Certificate::get(output_buffer& out) const
{
    return out << *this;
}


HandShakeType Certificate::get_type() const
{
    return certificate;
}


ServerDHParams::ServerDHParams()
    : pSz_(0), gSz_(0), pubSz_(0), p_(0), g_(0), Ys_(0)
{}


ServerDHParams::~ServerDHParams()
{
    ysArrayDelete(Ys_);
    ysArrayDelete(g_);
    ysArrayDelete(p_);
}


int ServerDHParams::get_pSize() const
{
    return pSz_;
}


int ServerDHParams::get_gSize() const
{
    return gSz_;
}


int ServerDHParams::get_pubSize() const
{
    return pubSz_;
}


const opaque* ServerDHParams::get_p() const
{
    return p_;
}


const opaque* ServerDHParams::get_g() const
{
    return g_;
}


const opaque* ServerDHParams::get_pub() const
{
    return Ys_;
}


opaque* ServerDHParams::alloc_p(int sz)
{
    p_ = NEW_YS opaque[pSz_ = sz];
    return p_;
}


opaque* ServerDHParams::alloc_g(int sz)
{
    g_ = NEW_YS opaque[gSz_ = sz];
    return g_;
}


opaque* ServerDHParams::alloc_pub(int sz)
{
    Ys_ = NEW_YS opaque[pubSz_ = sz];
    return Ys_;
}


int ServerKeyBase::get_length() const
{
    return 0;
}


opaque* ServerKeyBase::get_serverKey() const
{
    return 0;
}


// input operator for ServerHello
input_buffer& operator>>(input_buffer& input, ServerHello& hello)
{ 
    // Protocol
    hello.server_version_.major_ = input[AUTO];
    hello.server_version_.minor_ = input[AUTO];
   
    // Random
    input.read(hello.random_, RAN_LEN);
    
    // Session
    hello.id_len_ = input[AUTO];
    if (hello.id_len_)
        input.read(hello.session_id_, hello.id_len_);
 
    // Suites
    hello.cipher_suite_[0] = input[AUTO];
    hello.cipher_suite_[1] = input[AUTO];
   
    // Compression
    hello.compression_method_ = CompressionMethod(input[AUTO]);

    return input;
}


// output operator for ServerHello
output_buffer& operator<<(output_buffer& output, const ServerHello& hello)
{
    // Protocol
    output[AUTO] = hello.server_version_.major_;
    output[AUTO] = hello.server_version_.minor_;

    // Random
    output.write(hello.random_, RAN_LEN);

    // Session
    output[AUTO] = hello.id_len_;
    output.write(hello.session_id_, ID_LEN);

    // Suites
    output[AUTO] = hello.cipher_suite_[0];
    output[AUTO] = hello.cipher_suite_[1];

    // Compression
    output[AUTO] = hello.compression_method_;

    return output;
}


// Server Hello processing handler
void ServerHello::Process(input_buffer&, SSL& ssl)
{
    if (ssl.GetMultiProtocol()) {   // SSLv23 support
        if (ssl.isTLS() && server_version_.minor_ < 1)
            // downgrade to SSLv3
            ssl.useSecurity().use_connection().TurnOffTLS();
        else if (ssl.isTLSv1_1() && server_version_.minor_ == 1)
            // downdrage to TLSv1
            ssl.useSecurity().use_connection().TurnOffTLS1_1();
    }
    else if (ssl.isTLSv1_1() && server_version_.minor_ < 2) {
        ssl.SetError(badVersion_error);
        return;
    }
    else if (ssl.isTLS() && server_version_.minor_ < 1) {
        ssl.SetError(badVersion_error);
        return;
    }
    else if (!ssl.isTLS() && (server_version_.major_ == 3 &&
                              server_version_.minor_ >= 1)) {
        ssl.SetError(badVersion_error);
        return;
    }
    ssl.set_pending(cipher_suite_[1]);
    ssl.set_random(random_, server_end);
    if (id_len_)
        ssl.set_sessionID(session_id_);
    else
        ssl.useSecurity().use_connection().sessionID_Set_ = false;

    if (ssl.getSecurity().get_resuming())
        if (memcmp(session_id_, ssl.getSecurity().get_resume().GetID(),
                   ID_LEN) == 0) {
            ssl.set_masterSecret(ssl.getSecurity().get_resume().GetSecret());
            if (ssl.isTLS())
                ssl.deriveTLSKeys();
            else
                ssl.deriveKeys();
            ssl.useStates().useClient() = serverHelloDoneComplete;
            return;
        }
        else {
            ssl.useSecurity().set_resuming(false);
            ssl.useLog().Trace("server denied resumption");
        }

    if (ssl.CompressionOn() && !compression_method_)
        ssl.UnSetCompression(); // server isn't supporting yaSSL zlib request

    ssl.useStates().useClient() = serverHelloComplete;
}


ServerHello::ServerHello()
{
    memset(random_, 0, RAN_LEN);
    memset(session_id_, 0, ID_LEN);
}


ServerHello::ServerHello(ProtocolVersion pv, bool useCompression)
    : server_version_(pv),
      compression_method_(useCompression ? zlib : no_compression)
{
    memset(random_, 0, RAN_LEN);
    memset(session_id_, 0, ID_LEN);
}


input_buffer& ServerHello::set(input_buffer& in)
{
    return in  >> *this;
}


output_buffer& ServerHello::get(output_buffer& out) const
{
    return out << *this;
}


HandShakeType ServerHello::get_type() const
{
    return server_hello;
}


const opaque* ServerHello::get_random() const
{
    return random_;
}


// Server Hello Done processing handler
void ServerHelloDone::Process(input_buffer&, SSL& ssl)
{
    ssl.useStates().useClient() = serverHelloDoneComplete;
}


ServerHelloDone::ServerHelloDone()
{
    set_length(0);
}


input_buffer& ServerHelloDone::set(input_buffer& in)
{
    return in;
}


output_buffer& ServerHelloDone::get(output_buffer& out) const
{
    return out;
}


HandShakeType ServerHelloDone::get_type() const
{
    return server_hello_done;
}


int ClientKeyBase::get_length() const
{
    return 0;
}


opaque* ClientKeyBase::get_clientKey() const
{
    return 0;
}


// input operator for Client Hello
input_buffer& operator>>(input_buffer& input, ClientHello& hello)
{
    uint begin = input.get_current();  // could have extensions at end

    // Protocol
    hello.client_version_.major_ = input[AUTO];
    hello.client_version_.minor_ = input[AUTO];

    // Random
    input.read(hello.random_, RAN_LEN);

    // Session
    hello.id_len_ = input[AUTO];
    if (hello.id_len_) input.read(hello.session_id_, ID_LEN);
    
    // Suites
    byte   tmp[2];
    uint16 len;
    tmp[0] = input[AUTO];
    tmp[1] = input[AUTO];
    ato16(tmp, len);

    hello.suite_len_ = min(len, static_cast<uint16>(MAX_SUITE_SZ));
    input.read(hello.cipher_suites_, hello.suite_len_);
    if (len > hello.suite_len_)  // ignore extra suites
        input.set_current(input.get_current() + len - hello.suite_len_);

    // Compression
    hello.comp_len_ = input[AUTO];
    hello.compression_methods_ = no_compression;
    while (hello.comp_len_--) {
        CompressionMethod cm = CompressionMethod(input[AUTO]);
        if (cm == zlib)
            hello.compression_methods_ = zlib;
    }

    uint read = input.get_current() - begin;
    uint expected = hello.get_length();

    // ignore client hello extensions for now
    if (read < expected)
        input.set_current(input.get_current() + expected - read);

    return input;
}


// output operaotr for Client Hello
output_buffer& operator<<(output_buffer& output, const ClientHello& hello)
{ 
    // Protocol
    output[AUTO] = hello.client_version_.major_;
    output[AUTO] = hello.client_version_.minor_;

    // Random
    output.write(hello.random_, RAN_LEN);

    // Session
    output[AUTO] = hello.id_len_;
    if (hello.id_len_) output.write(hello.session_id_, ID_LEN);

    // Suites
    byte tmp[2];
    c16toa(hello.suite_len_, tmp);
    output[AUTO] = tmp[0];
    output[AUTO] = tmp[1];
    output.write(hello.cipher_suites_, hello.suite_len_);
  
    // Compression
    output[AUTO] = hello.comp_len_;
    output[AUTO] = hello.compression_methods_;

    return output;
}


// Client Hello processing handler
void ClientHello::Process(input_buffer&, SSL& ssl)
{
    // store version for pre master secret
    ssl.useSecurity().use_connection().chVersion_ = client_version_;

    if (client_version_.major_ != 3) {
        ssl.SetError(badVersion_error);
        return;
    }
    if (ssl.GetMultiProtocol()) {   // SSLv23 support
        if (ssl.isTLS() && client_version_.minor_ < 1) {
            // downgrade to SSLv3
            ssl.useSecurity().use_connection().TurnOffTLS();
            ProtocolVersion pv = ssl.getSecurity().get_connection().version_;
            ssl.useSecurity().use_parms().SetSuites(pv);  // reset w/ SSL suites
        }
        else if (ssl.isTLSv1_1() && client_version_.minor_ == 1)
            // downgrade to TLSv1, but use same suites
            ssl.useSecurity().use_connection().TurnOffTLS1_1();
    }
    else if (ssl.isTLSv1_1() && client_version_.minor_ < 2) {
        ssl.SetError(badVersion_error);
        return;
    }
    else if (ssl.isTLS() && client_version_.minor_ < 1) {
        ssl.SetError(badVersion_error);
        return;
    }
    else if (!ssl.isTLS() && client_version_.minor_ >= 1) {
        ssl.SetError(badVersion_error);
        return;
    }

    ssl.set_random(random_, client_end);

    while (id_len_) {  // trying to resume
        SSL_SESSION* session = 0;
        if (!ssl.getSecurity().GetContext()->GetSessionCacheOff())
            session = GetSessions().lookup(session_id_);
        if (!session)  {
            ssl.useLog().Trace("session lookup failed");
            break;
        }
        ssl.set_session(session);
        ssl.useSecurity().set_resuming(true);
        ssl.matchSuite(session->GetSuite(), SUITE_LEN);
        ssl.set_pending(ssl.getSecurity().get_parms().suite_[1]);
        ssl.set_masterSecret(session->GetSecret());

        opaque serverRandom[RAN_LEN];
        ssl.getCrypto().get_random().Fill(serverRandom, sizeof(serverRandom));
        ssl.set_random(serverRandom, server_end);
        if (ssl.isTLS())
            ssl.deriveTLSKeys();
        else
            ssl.deriveKeys();
        ssl.useStates().useServer() = clientKeyExchangeComplete;
        return;
    }
    ssl.matchSuite(cipher_suites_, suite_len_);
    ssl.set_pending(ssl.getSecurity().get_parms().suite_[1]);

    if (compression_methods_ == zlib)
        ssl.SetCompression();

    ssl.useStates().useServer() = clientHelloComplete;
}


input_buffer& ClientHello::set(input_buffer& in)
{
    return in  >> *this;
}


output_buffer& ClientHello::get(output_buffer& out) const
{
    return out << *this;
}


HandShakeType ClientHello::get_type() const
{
    return client_hello;
}


const opaque* ClientHello::get_random() const
{
    return random_;
}


ClientHello::ClientHello()
{
    memset(random_, 0, RAN_LEN);
}


ClientHello::ClientHello(ProtocolVersion pv, bool useCompression)
    : client_version_(pv),
      compression_methods_(useCompression ? zlib : no_compression)
{
    memset(random_, 0, RAN_LEN);
}


// output operator for ServerKeyExchange
output_buffer& operator<<(output_buffer& output, const ServerKeyExchange& sk)
{
    output.write(sk.getKey(), sk.getKeyLength());
    return output;
}


// Server Key Exchange processing handler
void ServerKeyExchange::Process(input_buffer& input, SSL& ssl)
{
    createKey(ssl);
    if (ssl.GetError()) return;
    server_key_->read(ssl, input);

    ssl.useStates().useClient() = serverKeyExchangeComplete;
}


ServerKeyExchange::ServerKeyExchange(SSL& ssl)
{
    createKey(ssl);
}


ServerKeyExchange::ServerKeyExchange()
    : server_key_(0)
{}


ServerKeyExchange::~ServerKeyExchange()
{
    ysDelete(server_key_);
}


void ServerKeyExchange::build(SSL& ssl) 
{ 
    server_key_->build(ssl); 
    set_length(server_key_->get_length());
}


const opaque* ServerKeyExchange::getKey() const
{
    return server_key_->get_serverKey();
}


int ServerKeyExchange::getKeyLength() const
{
    return server_key_->get_length();
}


input_buffer& ServerKeyExchange::set(input_buffer& in)
{
    return in;      // process does
}


output_buffer& ServerKeyExchange::get(output_buffer& out) const
{
    return out << *this;
}


HandShakeType ServerKeyExchange::get_type() const
{
    return server_key_exchange;
}


// CertificateRequest 
CertificateRequest::CertificateRequest()
    : typeTotal_(0)
{
    memset(certificate_types_, 0, sizeof(certificate_types_));
}


CertificateRequest::~CertificateRequest()
{

    STL::for_each(certificate_authorities_.begin(),
                  certificate_authorities_.end(),
                  del_ptr_zero()) ;
}


void CertificateRequest::Build()
{
    certificate_types_[0] = rsa_sign;
    certificate_types_[1] = dss_sign;

    typeTotal_ = 2;

    uint16 authCount = 0;
    uint16 authSz = 0;
  
    for (int j = 0; j < authCount; j++) {
        int sz = REQUEST_HEADER + MIN_DIS_SIZE;
        DistinguishedName dn;
        certificate_authorities_.push_back(dn = NEW_YS byte[sz]);

        opaque tmp[REQUEST_HEADER];
        c16toa(MIN_DIS_SIZE, tmp);
        memcpy(dn, tmp, sizeof(tmp));
  
        // fill w/ junk for now
        memcpy(dn, tmp, MIN_DIS_SIZE);
        authSz += sz;
    }

    set_length(SIZEOF_ENUM + typeTotal_ + REQUEST_HEADER + authSz);
}


input_buffer& CertificateRequest::set(input_buffer& in)
{
    return in >> *this;
}


output_buffer& CertificateRequest::get(output_buffer& out) const
{
    return out << *this;
}


// input operator for CertificateRequest
input_buffer& operator>>(input_buffer& input, CertificateRequest& request)
{
    // types
    request.typeTotal_ = input[AUTO];
    for (int i = 0; i < request.typeTotal_; i++)
        request.certificate_types_[i] = ClientCertificateType(input[AUTO]);

    byte tmp[REQUEST_HEADER];
    input.read(tmp, sizeof(tmp));
    uint16 sz;
    ato16(tmp, sz);

    // authorities
    while (sz) {
        uint16 dnSz;
        input.read(tmp, sizeof(tmp));
        ato16(tmp, dnSz);
        
        DistinguishedName dn;
        request.certificate_authorities_.push_back(dn = NEW_YS 
                                                  byte[REQUEST_HEADER + dnSz]);
        memcpy(dn, tmp, REQUEST_HEADER);
        input.read(&dn[REQUEST_HEADER], dnSz);

        sz -= dnSz + REQUEST_HEADER;
    }

    return input;
}


// output operator for CertificateRequest
output_buffer& operator<<(output_buffer& output,
                          const CertificateRequest& request)
{
    // types
    output[AUTO] = request.typeTotal_;
    for (int i = 0; i < request.typeTotal_; i++)
        output[AUTO] = request.certificate_types_[i];

    // authorities
    opaque tmp[REQUEST_HEADER];
    c16toa(request.get_length() - SIZEOF_ENUM -
           request.typeTotal_ - REQUEST_HEADER, tmp);
    output.write(tmp, sizeof(tmp));

    STL::list<DistinguishedName>::const_iterator first =
                                    request.certificate_authorities_.begin();
    STL::list<DistinguishedName>::const_iterator last =
                                    request.certificate_authorities_.end();
    while (first != last) {
        uint16 sz;
        ato16(*first, sz);
        output.write(*first, sz + REQUEST_HEADER);

        ++first;
    }

    return output;
}


// CertificateRequest processing handler
void CertificateRequest::Process(input_buffer&, SSL& ssl)
{
    CertManager& cm = ssl.useCrypto().use_certManager();

    // make sure user provided cert and key before sending and using
    if (cm.get_cert() && cm.get_privateKey())
        cm.setSendVerify();
}


HandShakeType CertificateRequest::get_type() const
{
    return certificate_request;
}


// CertificateVerify 
CertificateVerify::CertificateVerify() : signature_(0)
{}


CertificateVerify::~CertificateVerify()
{
    ysArrayDelete(signature_);
}


void CertificateVerify::Build(SSL& ssl)
{
    build_certHashes(ssl, hashes_);

    uint16 sz = 0;
    byte   len[VERIFY_HEADER];
    mySTL::auto_array<byte> sig;

    // sign
    const CertManager& cert = ssl.getCrypto().get_certManager();
    if (cert.get_keyType() == rsa_sa_algo) {
        RSA rsa(cert.get_privateKey(), cert.get_privateKeyLength(), false);

        sz = rsa.get_cipherLength() + VERIFY_HEADER;
        sig.reset(NEW_YS byte[sz]);

        c16toa(sz - VERIFY_HEADER, len);
        memcpy(sig.get(), len, VERIFY_HEADER);
        rsa.sign(sig.get() + VERIFY_HEADER, hashes_.md5_, sizeof(Hashes),
                 ssl.getCrypto().get_random());
    }
    else {  // DSA
        DSS dss(cert.get_privateKey(), cert.get_privateKeyLength(), false);

        sz = DSS_SIG_SZ + DSS_ENCODED_EXTRA + VERIFY_HEADER;
        sig.reset(NEW_YS byte[sz]);

        c16toa(sz - VERIFY_HEADER, len);
        memcpy(sig.get(), len, VERIFY_HEADER);
        dss.sign(sig.get() + VERIFY_HEADER, hashes_.sha_, SHA_LEN,
                 ssl.getCrypto().get_random());

        byte encoded[DSS_SIG_SZ + DSS_ENCODED_EXTRA];
        TaoCrypt::EncodeDSA_Signature(sig.get() + VERIFY_HEADER, encoded);
        memcpy(sig.get() + VERIFY_HEADER, encoded, sizeof(encoded));
    }
    set_length(sz);
    signature_ = sig.release();
}


input_buffer& CertificateVerify::set(input_buffer& in)
{
    return in >> *this;
}


output_buffer& CertificateVerify::get(output_buffer& out) const
{
    return out << *this;
}


// input operator for CertificateVerify
input_buffer& operator>>(input_buffer& input, CertificateVerify& request)
{
    byte tmp[VERIFY_HEADER];
    input.read(tmp, sizeof(tmp));

    uint16 sz = 0;
    ato16(tmp, sz);
    request.set_length(sz);

    request.signature_ = NEW_YS byte[sz];
    input.read(request.signature_, sz);

    return input;
}


// output operator for CertificateVerify
output_buffer& operator<<(output_buffer& output,
                          const CertificateVerify& verify)
{
    output.write(verify.signature_, verify.get_length());

    return output;
}


// CertificateVerify processing handler
void CertificateVerify::Process(input_buffer&, SSL& ssl)
{
    const Hashes&      hashVerify = ssl.getHashes().get_certVerify();
    const CertManager& cert       = ssl.getCrypto().get_certManager();

    if (cert.get_peerKeyType() == rsa_sa_algo) {
        RSA rsa(cert.get_peerKey(), cert.get_peerKeyLength());

        if (!rsa.verify(hashVerify.md5_, sizeof(hashVerify), signature_,
                        get_length()))
            ssl.SetError(verify_error);
    }
    else { // DSA
        byte decodedSig[DSS_SIG_SZ];
        TaoCrypt::DecodeDSA_Signature(decodedSig, signature_, get_length());
        
        DSS dss(cert.get_peerKey(), cert.get_peerKeyLength());
        if (!dss.verify(hashVerify.sha_, SHA_LEN, decodedSig, get_length()))
            ssl.SetError(verify_error);
    }
}


HandShakeType CertificateVerify::get_type() const
{
    return certificate_verify;
}


// output operator for ClientKeyExchange
output_buffer& operator<<(output_buffer& output, const ClientKeyExchange& ck)
{
    output.write(ck.getKey(), ck.getKeyLength());
    return output;
}


// Client Key Exchange processing handler
void ClientKeyExchange::Process(input_buffer& input, SSL& ssl)
{
    createKey(ssl);
    if (ssl.GetError()) return;
    client_key_->read(ssl, input);

    if (ssl.getCrypto().get_certManager().verifyPeer())
        build_certHashes(ssl, ssl.useHashes().use_certVerify());

    ssl.useStates().useServer() = clientKeyExchangeComplete;
}


ClientKeyExchange::ClientKeyExchange(SSL& ssl)
{
    createKey(ssl);
}


ClientKeyExchange::ClientKeyExchange()
    : client_key_(0)
{}


ClientKeyExchange::~ClientKeyExchange()
{
    ysDelete(client_key_);
}


void ClientKeyExchange::build(SSL& ssl) 
{ 
    client_key_->build(ssl); 
    set_length(client_key_->get_length());
}

const opaque* ClientKeyExchange::getKey() const
{
    return client_key_->get_clientKey();
}


int ClientKeyExchange::getKeyLength() const
{
    return client_key_->get_length();
}


input_buffer& ClientKeyExchange::set(input_buffer& in)
{
    return in;
}


output_buffer& ClientKeyExchange::get(output_buffer& out) const
{
    return out << *this;
}


HandShakeType ClientKeyExchange::get_type() const
{
    return client_key_exchange;
}


// input operator for Finished
input_buffer& operator>>(input_buffer& input, Finished&)
{
    /*  do in process */

    return input; 
}

// output operator for Finished
output_buffer& operator<<(output_buffer& output, const Finished& fin)
{
    if (fin.get_length() == FINISHED_SZ) {
        output.write(fin.hashes_.md5_, MD5_LEN);
        output.write(fin.hashes_.sha_, SHA_LEN);
    }
    else    // TLS_FINISHED_SZ
        output.write(fin.hashes_.md5_, TLS_FINISHED_SZ);

    return output;
}


// Finished processing handler
void Finished::Process(input_buffer& input, SSL& ssl)
{
    // verify hashes
    const  Finished& verify = ssl.getHashes().get_verify();
    uint finishedSz = ssl.isTLS() ? TLS_FINISHED_SZ : FINISHED_SZ;
    
    input.read(hashes_.md5_, finishedSz);

    if (memcmp(&hashes_, &verify.hashes_, finishedSz)) {
        ssl.SetError(verify_error);
        return;
    }

    // read verify mac
    opaque verifyMAC[SHA_LEN];
    uint macSz = finishedSz + HANDSHAKE_HEADER;

    if (ssl.isTLS())
        TLS_hmac(ssl, verifyMAC, input.get_buffer() + input.get_current()
                 - macSz, macSz, handshake, true);
    else
        hmac(ssl, verifyMAC, input.get_buffer() + input.get_current() - macSz,
             macSz, handshake, true);

    // read mac and fill
    opaque mac[SHA_LEN];   // max size
    int    digestSz = ssl.getCrypto().get_digest().get_digestSize();
    input.read(mac, digestSz);

    uint ivExtra = 0;
    if (ssl.getSecurity().get_parms().cipher_type_ == block)
        if (ssl.isTLSv1_1())
            ivExtra = ssl.getCrypto().get_cipher().get_blockSize();

    opaque fill;
    int    padSz = ssl.getSecurity().get_parms().encrypt_size_ - ivExtra -
                     HANDSHAKE_HEADER - finishedSz - digestSz;
    for (int i = 0; i < padSz; i++) 
        fill = input[AUTO];

    // verify mac
    if (memcmp(mac, verifyMAC, digestSz)) {
        ssl.SetError(verify_error);
        return;
    }

    // update states
    ssl.useStates().useHandShake() = handShakeReady;
    if (ssl.getSecurity().get_parms().entity_ == client_end)
        ssl.useStates().useClient() = serverFinishedComplete;
    else
        ssl.useStates().useServer() = clientFinishedComplete;
}


Finished::Finished()
{
    set_length(FINISHED_SZ);
}


uint8* Finished::set_md5()
{
    return hashes_.md5_;
}


uint8* Finished::set_sha()
{
    return hashes_.sha_;
}


input_buffer& Finished::set(input_buffer& in)
{
    return in  >> *this;
}


output_buffer& Finished::get(output_buffer& out) const
{
    return out << *this;
}


HandShakeType Finished::get_type() const
{
    return finished;
}


void clean(volatile opaque* p, uint sz, RandomPool& ran)
{
    uint i(0);

    for (i = 0; i < sz; ++i)
        p[i] = 0;

    ran.Fill(const_cast<opaque*>(p), sz);

    for (i = 0; i < sz; ++i)
        p[i] = 0;
}



Connection::Connection(ProtocolVersion v, RandomPool& ran)
    : pre_master_secret_(0), sequence_number_(0), peer_sequence_number_(0),
      pre_secret_len_(0), send_server_key_(false), master_clean_(false),
      TLS_(v.major_ >= 3 && v.minor_ >= 1),
      TLSv1_1_(v.major_ >= 3 && v.minor_ >= 2), compression_(false),
      version_(v), random_(ran)
{
    memset(sessionID_, 0, sizeof(sessionID_));
}


Connection::~Connection() 
{ 
    CleanMaster(); CleanPreMaster(); ysArrayDelete(pre_master_secret_);
}


void Connection::AllocPreSecret(uint sz) 
{ 
    pre_master_secret_ = NEW_YS opaque[pre_secret_len_ = sz];
}


void Connection::TurnOffTLS()
{
    TLS_ = false;
    version_.minor_ = 0;
}


void Connection::TurnOffTLS1_1()
{
    TLSv1_1_ = false;
    version_.minor_ = 1;
}


// wipeout master secret
void Connection::CleanMaster()
{
    if (!master_clean_) {
        volatile opaque* p = master_secret_;
        clean(p, SECRET_LEN, random_);
        master_clean_ = true;
    }
}


// wipeout pre master secret
void Connection::CleanPreMaster()
{
    if (pre_master_secret_) {
        volatile opaque* p = pre_master_secret_;
        clean(p, pre_secret_len_, random_);

        ysArrayDelete(pre_master_secret_);
        pre_master_secret_ = 0;
    }
}


// Create functions for message factory
Message* CreateCipherSpec() { return NEW_YS ChangeCipherSpec; }
Message* CreateAlert()      { return NEW_YS Alert; }
Message* CreateHandShake()  { return NEW_YS HandShakeHeader; }
Message* CreateData()       { return NEW_YS Data; }

// Create functions for handshake factory
HandShakeBase* CreateHelloRequest()       { return NEW_YS HelloRequest; }
HandShakeBase* CreateClientHello()        { return NEW_YS ClientHello; }
HandShakeBase* CreateServerHello()        { return NEW_YS ServerHello; }
HandShakeBase* CreateCertificate()        { return NEW_YS Certificate; }
HandShakeBase* CreateServerKeyExchange()  { return NEW_YS ServerKeyExchange;}
HandShakeBase* CreateCertificateRequest() { return NEW_YS 
                                                    CertificateRequest; }
HandShakeBase* CreateServerHelloDone()    { return NEW_YS ServerHelloDone; }
HandShakeBase* CreateCertificateVerify()  { return NEW_YS CertificateVerify;}
HandShakeBase* CreateClientKeyExchange()  { return NEW_YS ClientKeyExchange;}
HandShakeBase* CreateFinished()           { return NEW_YS Finished; }

// Create functions for server key exchange factory
ServerKeyBase* CreateRSAServerKEA()       { return NEW_YS RSA_Server; }
ServerKeyBase* CreateDHServerKEA()        { return NEW_YS DH_Server; }
ServerKeyBase* CreateFortezzaServerKEA()  { return NEW_YS Fortezza_Server; }

// Create functions for client key exchange factory
ClientKeyBase* CreateRSAClient()      { return NEW_YS 
                                                EncryptedPreMasterSecret; }
ClientKeyBase* CreateDHClient()       { return NEW_YS 
                                                ClientDiffieHellmanPublic; }
ClientKeyBase* CreateFortezzaClient() { return NEW_YS FortezzaKeys; }


// Constructor calls this to Register compile time callbacks
void InitMessageFactory(MessageFactory& mf)
{
    mf.Reserve(4);
    mf.Register(alert, CreateAlert);
    mf.Register(change_cipher_spec, CreateCipherSpec);
    mf.Register(handshake, CreateHandShake);
    mf.Register(application_data, CreateData);
}


// Constructor calls this to Register compile time callbacks
void InitHandShakeFactory(HandShakeFactory& hsf)
{
    hsf.Reserve(10);
    hsf.Register(hello_request, CreateHelloRequest);
    hsf.Register(client_hello, CreateClientHello);
    hsf.Register(server_hello, CreateServerHello);
    hsf.Register(certificate, CreateCertificate);
    hsf.Register(server_key_exchange, CreateServerKeyExchange);
    hsf.Register(certificate_request, CreateCertificateRequest);
    hsf.Register(server_hello_done, CreateServerHelloDone);
    hsf.Register(certificate_verify, CreateCertificateVerify);
    hsf.Register(client_key_exchange, CreateClientKeyExchange);
    hsf.Register(finished, CreateFinished);
}


// Constructor calls this to Register compile time callbacks
void InitServerKeyFactory(ServerKeyFactory& skf)
{
    skf.Reserve(3);
    skf.Register(rsa_kea, CreateRSAServerKEA);
    skf.Register(diffie_hellman_kea, CreateDHServerKEA);
    skf.Register(fortezza_kea, CreateFortezzaServerKEA);
}


// Constructor calls this to Register compile time callbacks
void InitClientKeyFactory(ClientKeyFactory& ckf)
{
    ckf.Reserve(3);
    ckf.Register(rsa_kea, CreateRSAClient);
    ckf.Register(diffie_hellman_kea, CreateDHClient);
    ckf.Register(fortezza_kea, CreateFortezzaClient);
}


} // namespace

/* yassl_imp.hpp                                
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

/*  yaSSL implementation header defines all strucutres from the SSL.v3 
 *  specification "draft-freier-ssl-version3-02.txt"
 *  all page citations refer to this document unless otherwise noted.
 */


#ifndef yaSSL_IMP_HPP
#define yaSSL_IMP_HPP

#ifdef _MSC_VER
    // disable truncated debug symbols
    #pragma warning(disable:4786)
#endif

#include "yassl_types.hpp"
#include "factory.hpp"
#include STL_LIST_FILE


namespace STL = STL_NAMESPACE;


namespace yaSSL {


class SSL;              // forward decls
class input_buffer;
class output_buffer;


struct ProtocolVersion {
    uint8 major_;
    uint8 minor_;     // major and minor SSL/TLS version numbers

    ProtocolVersion(uint8 maj = 3, uint8 min = 0);
};


// Record Layer Header for PlainText, Compressed, and CipherText
struct RecordLayerHeader {
    ContentType     type_;
    ProtocolVersion version_;
    uint16          length_;             // should not exceed 2^14
};


// base for all messages
struct Message : public virtual_base {
    virtual input_buffer& set(input_buffer&) =0;   
    virtual output_buffer& get(output_buffer&) const =0;

    virtual void Process(input_buffer&, SSL&) =0;
    virtual ContentType get_type() const =0;
    virtual uint16      get_length() const =0;

    virtual ~Message() {}
};


class ChangeCipherSpec : public Message {
    CipherChoice type_;
public:
    ChangeCipherSpec();

    friend input_buffer& operator>>(input_buffer&, ChangeCipherSpec&);
    friend output_buffer& operator<<(output_buffer&, const ChangeCipherSpec&);

    input_buffer& set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    ContentType get_type()   const;
    uint16      get_length() const;
    void Process(input_buffer&, SSL&);
private:
    ChangeCipherSpec(const ChangeCipherSpec&);            // hide copy
    ChangeCipherSpec& operator=(const ChangeCipherSpec&); // and assign
};



class Alert : public Message {
    AlertLevel       level_;
    AlertDescription description_;
public:
    Alert() {}
    Alert(AlertLevel al, AlertDescription ad);

    ContentType get_type()   const;
    uint16      get_length() const;
    void Process(input_buffer&, SSL&);

    friend input_buffer& operator>>(input_buffer&, Alert&);
    friend output_buffer& operator<<(output_buffer&, const Alert&);
   
    input_buffer& set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;
private:
    Alert(const Alert&);            // hide copy
    Alert& operator=(const Alert&); // and assign
};


class Data : public Message {
    uint16        length_;
    opaque*       buffer_;         // read  buffer used by fillData input
    const opaque* write_buffer_;   // write buffer used by output operator
public:
    Data();
    Data(uint16 len, opaque* b);

    friend output_buffer& operator<<(output_buffer&, const Data&);

    input_buffer& set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    ContentType   get_type()     const;
    uint16        get_length()   const;
    void          set_length(uint16 l);
    opaque*       set_buffer();
    void          SetData(uint16, const opaque*);
    void Process(input_buffer&, SSL&);
private:
    Data(const Data&);            // hide copy
    Data& operator=(const Data&); // and assign
};


uint32 c24to32(const uint24);       // forward form internal header
void   c32to24(uint32, uint24&);


// HandShake header, same for each message type from page 20/21
class HandShakeHeader : public Message {
    HandShakeType      type_;
    uint24             length_;      // length of message
public:
    HandShakeHeader() {}

    ContentType   get_type()   const;
    uint16        get_length() const;
    HandShakeType get_handshakeType() const;
    void Process(input_buffer&, SSL&);

    void set_type(HandShakeType hst);
    void set_length(uint32 u32);

    friend input_buffer& operator>>(input_buffer&, HandShakeHeader&);
    friend output_buffer& operator<<(output_buffer&, const HandShakeHeader&);

    input_buffer& set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;
private:
    HandShakeHeader(const HandShakeHeader&);            // hide copy
    HandShakeHeader& operator=(const HandShakeHeader&); // and assign
};


// Base Class for all handshake messages
class HandShakeBase : public virtual_base {
    int     length_;
public:
    int     get_length() const;
    void    set_length(int);

    // for building buffer's type field
    virtual HandShakeType get_type() const =0;                

    // handles dispactch of proper >>
    virtual input_buffer&  set(input_buffer& in) =0;
    virtual output_buffer& get(output_buffer& out) const =0;

    virtual void Process(input_buffer&, SSL&) =0;

    virtual ~HandShakeBase() {}
};


struct HelloRequest : public HandShakeBase {
    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    void Process(input_buffer&, SSL&);

    HandShakeType get_type() const;
};


// The Client's Hello Message from page 23
class ClientHello : public HandShakeBase {
    ProtocolVersion     client_version_;
    Random              random_;
    uint8               id_len_;                         // session id length
    opaque              session_id_[ID_LEN];
    uint16              suite_len_;                      // cipher suite length
    opaque              cipher_suites_[MAX_SUITE_SZ];
    uint8               comp_len_;                       // compression length
    CompressionMethod   compression_methods_;  
public:
    friend input_buffer&  operator>>(input_buffer&, ClientHello&);
    friend output_buffer& operator<<(output_buffer&, const ClientHello&);
  
    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    HandShakeType  get_type() const;
    void Process(input_buffer&, SSL&);

    const opaque* get_random() const;
    friend void buildClientHello(SSL&, ClientHello&);
    friend void ProcessOldClientHello(input_buffer& input, SSL& ssl);

    ClientHello();
    ClientHello(ProtocolVersion pv, bool useCompression);
private:
    ClientHello(const ClientHello&);            // hide copy
    ClientHello& operator=(const ClientHello&); // and assign
};



// The Server's Hello Message from page 24
class ServerHello : public HandShakeBase {
    ProtocolVersion     server_version_;
    Random              random_;
    uint8               id_len_;                 // session id length
    opaque              session_id_[ID_LEN];
    opaque              cipher_suite_[SUITE_LEN];
    CompressionMethod   compression_method_;
public:
    ServerHello(ProtocolVersion pv, bool useCompression);
    ServerHello();
          
    friend input_buffer&  operator>>(input_buffer&, ServerHello&);
    friend output_buffer& operator<<(output_buffer&, const ServerHello&);
   
    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    HandShakeType  get_type() const;
    void Process(input_buffer&, SSL&);

    const opaque* get_random() const;
    friend void buildServerHello(SSL&, ServerHello&);
private:
    ServerHello(const ServerHello&);            // hide copy
    ServerHello& operator=(const ServerHello&); // and assign
};


class x509;  

// Certificate could be a chain
class Certificate : public HandShakeBase {
    const x509* cert_;
public:
    Certificate();
    explicit Certificate(const x509* cert); 
    friend output_buffer& operator<<(output_buffer&, const Certificate&);

    const opaque* get_buffer() const;
  
    // Process handles input, needs SSL
    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    HandShakeType get_type() const;
    void Process(input_buffer&, SSL&);
private:
    Certificate(const Certificate&);            // hide copy
    Certificate& operator=(const Certificate&); // and assign
};



// RSA Public Key
struct ServerRSAParams {
    opaque* rsa_modulus_;
    opaque* rsa_exponent_;
};


// Ephemeral Diffie-Hellman Parameters
class ServerDHParams {
    int pSz_;
    int gSz_;
    int pubSz_;
    opaque* p_;
    opaque* g_;
    opaque* Ys_;
public:
    ServerDHParams();
    ~ServerDHParams();

    int get_pSize()   const;
    int get_gSize()   const;
    int get_pubSize() const;

    const opaque* get_p()   const;
    const opaque* get_g()   const;
    const opaque* get_pub() const;

    opaque* alloc_p(int sz);
    opaque* alloc_g(int sz);
    opaque* alloc_pub(int sz);
private:
    ServerDHParams(const ServerDHParams&);            // hide copy
    ServerDHParams& operator=(const ServerDHParams&); // and assign
};


struct ServerKeyBase : public virtual_base {
    virtual ~ServerKeyBase() {}
    virtual void build(SSL&) {}
    virtual void read(SSL&, input_buffer&) {}
    virtual int  get_length() const;     
    virtual opaque* get_serverKey() const;
};


// Server random number for FORTEZZA KEA
struct Fortezza_Server : public ServerKeyBase {
    opaque r_s_[FORTEZZA_MAX];
};


struct SignatureBase : public virtual_base {
    virtual ~SignatureBase() {}
};

struct anonymous_sa : public SignatureBase {};


struct Hashes {
    uint8 md5_[MD5_LEN];
    uint8 sha_[SHA_LEN];
};
    

struct rsa_sa : public SignatureBase {
    Hashes hashes_;
};


struct dsa_sa : public SignatureBase {
    uint8 sha_[SHA_LEN];
};


// Server's Diffie-Hellman exchange
class DH_Server : public ServerKeyBase {
    ServerDHParams  parms_;
    opaque*         signature_;

    int             length_;                // total length of message
    opaque*         keyMessage_;            // total exchange message
public:
    DH_Server();
    ~DH_Server();

    void build(SSL&);
    void read(SSL&, input_buffer&);
    int  get_length() const;
    opaque* get_serverKey() const;
private:
    DH_Server(const DH_Server&);            // hide copy
    DH_Server& operator=(const DH_Server&); // and assign
};


// Server's RSA exchange
struct RSA_Server : public ServerKeyBase {
    ServerRSAParams params_;
    opaque*         signature_;   // signed rsa_sa hashes
};


class ServerKeyExchange : public HandShakeBase {
    ServerKeyBase* server_key_;
public:
    explicit ServerKeyExchange(SSL&);
    ServerKeyExchange();
    ~ServerKeyExchange();

    void createKey(SSL&);
    void build(SSL& ssl);
   
    const opaque* getKey()       const;
    int           getKeyLength() const;

    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    friend output_buffer& operator<<(output_buffer&, const ServerKeyExchange&);

    void Process(input_buffer&, SSL&);
    HandShakeType get_type() const;
private:
    ServerKeyExchange(const ServerKeyExchange&);            // hide copy
    ServerKeyExchange& operator=(const ServerKeyExchange&); // and assign
};



class CertificateRequest : public HandShakeBase  {
    ClientCertificateType         certificate_types_[CERT_TYPES];
    int                           typeTotal_;
    STL::list<DistinguishedName>  certificate_authorities_;
public:
    CertificateRequest();
    ~CertificateRequest();

    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    friend input_buffer&  operator>>(input_buffer&, CertificateRequest&);
    friend output_buffer& operator<<(output_buffer&,
                                     const CertificateRequest&);

    void Process(input_buffer&, SSL&);
    HandShakeType get_type() const;

    void Build();
private:
    CertificateRequest(const CertificateRequest&);              // hide copy
    CertificateRequest& operator=(const CertificateRequest&);   // and assign
};


struct ServerHelloDone : public HandShakeBase {
    ServerHelloDone();
    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    void Process(input_buffer& input, SSL& ssl);

    HandShakeType get_type() const;
};


struct PreMasterSecret {
    opaque  random_[SECRET_LEN];     // first two bytes Protocol Version
};


struct ClientKeyBase : public virtual_base {
    virtual ~ClientKeyBase() {}
    virtual void build(SSL&) {}
    virtual void read(SSL&, input_buffer&) {}
    virtual int  get_length() const;
    virtual opaque* get_clientKey() const;
};


class EncryptedPreMasterSecret : public ClientKeyBase {
    opaque* secret_;
    int     length_;
public:
    EncryptedPreMasterSecret();
    ~EncryptedPreMasterSecret();

    void    build(SSL&);
    void    read(SSL&, input_buffer&);
    int     get_length()    const;
    opaque* get_clientKey() const;
    void    alloc(int sz);
private:
    // hide copy and assign
    EncryptedPreMasterSecret(const EncryptedPreMasterSecret&);           
    EncryptedPreMasterSecret& operator=(const EncryptedPreMasterSecret&);
};


// Fortezza Key Parameters from page 29
// hard code lengths cause only used here
struct FortezzaKeys : public ClientKeyBase {
    opaque  y_c_                      [128];    // client's Yc, public value
    opaque  r_c_                      [128];    // client's Rc
    opaque  y_signature_              [40];     // DSS signed public key
    opaque  wrapped_client_write_key_ [12];     // wrapped by the TEK
    opaque  wrapped_server_write_key_ [12];     // wrapped by the TEK
    opaque  client_write_iv_          [24];      
    opaque  server_write_iv_          [24];
    opaque  master_secret_iv_         [24];     // IV used to encrypt preMaster
    opaque  encrypted_preMasterSecret_[48];     // random & crypted by the TEK
};



// Diffie-Hellman public key from page 40/41
class  ClientDiffieHellmanPublic : public ClientKeyBase {
    PublicValueEncoding public_value_encoding_;
    int     length_;    // includes two byte length for message
    opaque* Yc_;        // length + Yc_
    // dh_Yc only if explicit, otherwise sent in certificate
    enum { KEY_OFFSET = 2 };
public:
    ClientDiffieHellmanPublic();
    ~ClientDiffieHellmanPublic();

    void    build(SSL&);
    void    read(SSL&, input_buffer&);
    int     get_length()    const;
    opaque* get_clientKey() const;
    void    alloc(int sz, bool offset = false);
private:
    // hide copy and assign
    ClientDiffieHellmanPublic(const ClientDiffieHellmanPublic&);
    ClientDiffieHellmanPublic& operator=(const ClientDiffieHellmanPublic&);
};


class ClientKeyExchange : public HandShakeBase {
    ClientKeyBase*  client_key_;
public:
    explicit ClientKeyExchange(SSL& ssl);
    ClientKeyExchange();
    ~ClientKeyExchange();

    void createKey(SSL&);
    void build(SSL& ssl);
   
    const opaque* getKey()       const;
    int           getKeyLength() const;

    friend output_buffer& operator<<(output_buffer&, const ClientKeyExchange&);
   
    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    HandShakeType  get_type() const;
    void Process(input_buffer&, SSL&);
private:
    ClientKeyExchange(const ClientKeyExchange&);            // hide copy
    ClientKeyExchange& operator=(const ClientKeyExchange&); // and assign
};


class CertificateVerify : public HandShakeBase {
    Hashes             hashes_;
    byte*              signature_;  // owns
public:
    CertificateVerify();
    ~CertificateVerify();

    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    friend input_buffer&  operator>>(input_buffer&, CertificateVerify&);
    friend output_buffer& operator<<(output_buffer&, const CertificateVerify&);

    void Process(input_buffer&, SSL&);
    HandShakeType get_type() const;

    void Build(SSL&);
private:
    CertificateVerify(const CertificateVerify&);              // hide copy
    CertificateVerify& operator=(const CertificateVerify&);   // and assign
};


class Finished : public HandShakeBase {
    Hashes hashes_;
public:
    Finished();

    uint8* set_md5();
    uint8* set_sha();

    friend input_buffer& operator>>(input_buffer&, Finished&);
    friend output_buffer& operator<<(output_buffer&, const Finished&);

    input_buffer&  set(input_buffer& in);
    output_buffer& get(output_buffer& out) const;

    void Process(input_buffer&, SSL&);

    HandShakeType get_type() const;
private:
    Finished(const Finished&);            // hide copy
    Finished& operator=(const Finished&); // and assign
};


class RandomPool;  // forward for connection


// SSL Connection defined on page 11
struct Connection {
    opaque          *pre_master_secret_;
    opaque          master_secret_[SECRET_LEN];
    opaque          client_random_[RAN_LEN];
    opaque          server_random_[RAN_LEN];
    opaque          sessionID_[ID_LEN];
    opaque          client_write_MAC_secret_[SHA_LEN]; // sha  is max size
    opaque          server_write_MAC_secret_[SHA_LEN];
    opaque          client_write_key_[AES_256_KEY_SZ]; // aes 256bit is max sz
    opaque          server_write_key_[AES_256_KEY_SZ];
    opaque          client_write_IV_[AES_IV_SZ];       // aes is max size
    opaque          server_write_IV_[AES_IV_SZ];
    uint32          sequence_number_;
    uint32          peer_sequence_number_;
    uint32          pre_secret_len_;                   // pre master length
    bool            send_server_key_;                  // server key exchange?
    bool            master_clean_;                     // master secret clean?
    bool            TLS_;                              // TLSv1 or greater
    bool            TLSv1_1_;                          // TLSv1.1 or greater
    bool            sessionID_Set_;                    // do we have a session
    bool            compression_;                      // zlib compression?
    ProtocolVersion version_;                          // negotiated version
    ProtocolVersion chVersion_;                        // client hello version
    RandomPool&     random_;

    Connection(ProtocolVersion v, RandomPool& ran);
    ~Connection();

    void AllocPreSecret(uint sz);
    void CleanPreMaster();
    void CleanMaster();
    void TurnOffTLS();
    void TurnOffTLS1_1();
private:
    Connection(const Connection&);              // hide copy
    Connection& operator=(const Connection&);   // and assign
};


struct Ciphers;   // forward


// TLSv1 Security Spec, defined on page 56 of RFC 2246
struct Parameters {
    ConnectionEnd        entity_;
    BulkCipherAlgorithm  bulk_cipher_algorithm_;
    CipherType           cipher_type_;
    uint8                key_size_;
    uint8                iv_size_;
    IsExportable         is_exportable_;
    MACAlgorithm         mac_algorithm_;
    uint8                hash_size_;
    CompressionMethod    compression_algorithm_;
    KeyExchangeAlgorithm kea_;                        // yassl additions
    SignatureAlgorithm   sig_algo_;                   // signature auth type
    SignatureAlgorithm   verify_algo_;                // cert verify auth type
    bool                 pending_;                  
    bool                 resumable_;                  // new conns by session
    uint16               encrypt_size_;               // current msg encrypt sz
    Cipher               suite_[SUITE_LEN];           // choosen suite
    uint8                suites_size_;
    Cipher               suites_[MAX_SUITE_SZ];
    char                 cipher_name_[MAX_SUITE_NAME];
    char                 cipher_list_[MAX_CIPHERS][MAX_SUITE_NAME];

    Parameters(ConnectionEnd, const Ciphers&, ProtocolVersion, bool haveDH);

    void SetSuites(ProtocolVersion pv, bool removeDH = false);
    void SetCipherNames();
private:
    Parameters(const Parameters&);              // hide copy
    Parameters& operator=(const Parameters&);   // and assing
};


input_buffer&  operator>>(input_buffer&,  RecordLayerHeader&);
output_buffer& operator<<(output_buffer&, const RecordLayerHeader&);

input_buffer&  operator>>(input_buffer&,  Message&);
output_buffer& operator<<(output_buffer&, const Message&);

input_buffer&  operator>>(input_buffer&,  HandShakeBase&);
output_buffer& operator<<(output_buffer&, const HandShakeBase&);


// Message Factory definition
// uses the ContentType enumeration for unique id
typedef Factory<Message> MessageFactory;
void    InitMessageFactory(MessageFactory&);     // registers derived classes

// HandShake Factory definition
// uses the HandShakeType enumeration for unique id
typedef Factory<HandShakeBase> HandShakeFactory;  
void    InitHandShakeFactory(HandShakeFactory&); // registers derived classes

// ServerKey Factory definition
// uses KeyExchangeAlgorithm enumeration for unique  id
typedef Factory<ServerKeyBase> ServerKeyFactory;
void    InitServerKeyFactory(ServerKeyFactory&);

// ClientKey Factory definition
// uses KeyExchangeAlgorithm enumeration for unique  id
typedef Factory<ClientKeyBase> ClientKeyFactory;
void    InitClientKeyFactory(ClientKeyFactory&);


// Message Creators
Message* CreateHandShake();
Message* CreateCipherSpec();
Message* CreateAlert();
Message* CreateData();


// HandShake Creators
HandShakeBase* CreateCertificate();
HandShakeBase* CreateHelloRequest();
HandShakeBase* CreateClientHello();
HandShakeBase* CreateServerHello();
HandShakeBase* CreateServerKeyExchange();
HandShakeBase* CreateCertificateRequest();
HandShakeBase* CreateServerHelloDone();
HandShakeBase* CreateClientKeyExchange();
HandShakeBase* CreateCertificateVerify();
HandShakeBase* CreateFinished();


// ServerKey Exchange Creators
ServerKeyBase* CreateRSAServerKEA();
ServerKeyBase* CreateDHServerKEA();
ServerKeyBase* CreateFortezzaServerKEA();

// ClientKey Exchange Creators
ClientKeyBase* CreateRSAClient();
ClientKeyBase* CreateDHClient();
ClientKeyBase* CreateFortezzaClient();



} // naemspace

#endif // yaSSL_IMP_HPP

 /* ssl.cpp                                
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

/*  SSL source implements all openssl compatibility API functions
 *
 *  TODO: notes are mostly api additions to allow compilation with mysql
 *  they don't affect normal modes but should be provided for completeness

 *  stunnel functions at end of file
 */



/*  see man pages for function descriptions */

#include "runtime.hpp"
#include "openssl/ssl.h"
#include "handshake.hpp"
#include "yassl_int.hpp"
#include "md5.hpp"              // for TaoCrypt MD5 size assert
#include "md4.hpp"              // for TaoCrypt MD4 size assert
#include "file.hpp"             // for TaoCrypt Source
#include "coding.hpp"           // HexDecoder
#include "helpers.hpp"          // for placement new hack
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>    // FindFirstFile etc..
#else
    #include <sys/types.h>  // file helper
    #include <sys/stat.h>   // stat
    #include <dirent.h>     // opendir
#endif


namespace yaSSL {



int read_file(SSL_CTX* ctx, const char* file, int format, CertType type)
{
    if (format != SSL_FILETYPE_ASN1 && format != SSL_FILETYPE_PEM)
        return SSL_BAD_FILETYPE;

    if (file == NULL || !file[0])
      return SSL_BAD_FILE;

    FILE* input = fopen(file, "rb");
    if (!input)
        return SSL_BAD_FILE;

    if (type == CA) {
        // may have a bunch of CAs
        x509* ptr;
        while ( (ptr = PemToDer(input, Cert)) )
            ctx->AddCA(ptr);

        if (!feof(input)) {
            fclose(input);
            return SSL_BAD_FILE;
        }
    }
    else {
        x509*& x = (type == Cert) ? ctx->certificate_ : ctx->privateKey_;

        if (format == SSL_FILETYPE_ASN1) {
            fseek(input, 0, SEEK_END);
            long sz = ftell(input);
            rewind(input);
            x = NEW_YS x509(sz); // takes ownership
            size_t bytes = fread(x->use_buffer(), sz, 1, input);
            if (bytes != 1) {
                fclose(input);
                return SSL_BAD_FILE;
            }
        }
        else {
            EncryptedInfo info;
            x = PemToDer(input, type, &info);
            if (!x) {
                fclose(input);
                return SSL_BAD_FILE;
            }
            if (info.set) {
                // decrypt
                char password[80];
                pem_password_cb cb = ctx->GetPasswordCb();
                if (!cb) {
                    fclose(input);
                    return SSL_BAD_FILE;
                }
                int passwordSz = cb(password, sizeof(password), 0,
                                    ctx->GetUserData());
                byte key[AES_256_KEY_SZ];  // max sizes
                byte iv[AES_IV_SZ];
                
                // use file's salt for key derivation, but not real iv
                TaoCrypt::Source source(info.iv, info.ivSz);
                TaoCrypt::HexDecoder dec(source);
                memcpy(info.iv, source.get_buffer(), min((uint)sizeof(info.iv),
                                                         source.size()));
                EVP_BytesToKey(info.name, "MD5", info.iv, (byte*)password,
                               passwordSz, 1, key, iv);

                mySTL::auto_ptr<BulkCipher> cipher;
                if (strncmp(info.name, "DES-CBC", 7) == 0)
                    cipher.reset(NEW_YS DES);
                else if (strncmp(info.name, "DES-EDE3-CBC", 13) == 0)
                    cipher.reset(NEW_YS DES_EDE);
                else if (strncmp(info.name, "AES-128-CBC", 13) == 0)
                    cipher.reset(NEW_YS AES(AES_128_KEY_SZ));
                else if (strncmp(info.name, "AES-192-CBC", 13) == 0)
                    cipher.reset(NEW_YS AES(AES_192_KEY_SZ));
                else if (strncmp(info.name, "AES-256-CBC", 13) == 0)
                    cipher.reset(NEW_YS AES(AES_256_KEY_SZ));
                else {
                    fclose(input);
                    return SSL_BAD_FILE;
                }
                cipher->set_decryptKey(key, info.iv);
                mySTL::auto_ptr<x509> newx(NEW_YS x509(x->get_length()));   
                cipher->decrypt(newx->use_buffer(), x->get_buffer(),
                                x->get_length());
                ysDelete(x);
                x = newx.release();
            }
        }
    }
    fclose(input);
    return SSL_SUCCESS;
}


extern "C" {


SSL_METHOD* SSLv3_method()
{
    return SSLv3_client_method();
}


SSL_METHOD* SSLv3_server_method()
{
    return NEW_YS SSL_METHOD(server_end, ProtocolVersion(3,0));
}


SSL_METHOD* SSLv3_client_method()
{
    return NEW_YS SSL_METHOD(client_end, ProtocolVersion(3,0));
}


SSL_METHOD* TLSv1_server_method()
{
    return NEW_YS SSL_METHOD(server_end, ProtocolVersion(3,1));
}


SSL_METHOD* TLSv1_client_method()
{
    return NEW_YS SSL_METHOD(client_end, ProtocolVersion(3,1));
}


SSL_METHOD* TLSv1_1_server_method()
{
    return NEW_YS SSL_METHOD(server_end, ProtocolVersion(3,2));
}


SSL_METHOD* TLSv1_1_client_method()
{
    return NEW_YS SSL_METHOD(client_end, ProtocolVersion(3,2));
}


SSL_METHOD* SSLv23_server_method()
{
    // compatibility only, no version 2 support, but does SSL 3 and TLS 1
    return NEW_YS SSL_METHOD(server_end, ProtocolVersion(3,2), true);
}


SSL_METHOD* SSLv23_client_method()
{
    // compatibility only, no version 2 support, but does SSL 3 and TLS 1
    // though it sends TLS1 hello not SSLv2 so SSLv3 only servers will decline
    // TODO: maybe add support to send SSLv2 hello ???
    return NEW_YS SSL_METHOD(client_end, ProtocolVersion(3,2), true);
}


SSL_CTX* SSL_CTX_new(SSL_METHOD* method)
{
    return NEW_YS SSL_CTX(method);
}


void SSL_CTX_free(SSL_CTX* ctx)
{
    ysDelete(ctx);
}


SSL* SSL_new(SSL_CTX* ctx)
{
    return NEW_YS SSL(ctx);
}


void SSL_free(SSL* ssl)
{
    ysDelete(ssl);
}


int SSL_set_fd(SSL* ssl, YASSL_SOCKET_T fd)
{
    ssl->useSocket().set_fd(fd);
    return SSL_SUCCESS;
}


YASSL_SOCKET_T SSL_get_fd(const SSL* ssl)
{
    return ssl->getSocket().get_fd();
}


// if you get an error from connect see note at top of README
int SSL_connect(SSL* ssl)
{
    if (ssl->GetError() == YasslError(SSL_ERROR_WANT_READ))
        ssl->SetError(no_error);

    ClientState neededState;

    switch (ssl->getStates().GetConnect()) {

    case CONNECT_BEGIN :
        sendClientHello(*ssl);
        if (!ssl->GetError())
            ssl->useStates().UseConnect() = CLIENT_HELLO_SENT;

    case CLIENT_HELLO_SENT :
        neededState = ssl->getSecurity().get_resuming() ?
                      serverFinishedComplete : serverHelloDoneComplete;
        while (ssl->getStates().getClient() < neededState) {
            if (ssl->GetError()) break;
            processReply(*ssl);
        }
        if (!ssl->GetError())
            ssl->useStates().UseConnect() = FIRST_REPLY_DONE;

    case FIRST_REPLY_DONE :
        if(ssl->getCrypto().get_certManager().sendVerify())
            sendCertificate(*ssl);

        if (!ssl->getSecurity().get_resuming())
            sendClientKeyExchange(*ssl);

        if(ssl->getCrypto().get_certManager().sendVerify())
            sendCertificateVerify(*ssl);

        sendChangeCipher(*ssl);
        sendFinished(*ssl, client_end);
        ssl->flushBuffer();

        if (!ssl->GetError())
            ssl->useStates().UseConnect() = FINISHED_DONE;

    case FINISHED_DONE :
        if (!ssl->getSecurity().get_resuming())
            while (ssl->getStates().getClient() < serverFinishedComplete) {
                if (ssl->GetError()) break;
                processReply(*ssl);
            }
        if (!ssl->GetError())
            ssl->useStates().UseConnect() = SECOND_REPLY_DONE;

    case SECOND_REPLY_DONE :
        ssl->verifyState(serverFinishedComplete);
        ssl->useLog().ShowTCP(ssl->getSocket().get_fd());

        if (ssl->GetError()) {
            GetErrors().Add(ssl->GetError());
            return SSL_FATAL_ERROR;
        }   
        return SSL_SUCCESS;

    default :
        return SSL_FATAL_ERROR; // unkown state
    }
}


int SSL_write(SSL* ssl, const void* buffer, int sz)
{
    return sendData(*ssl, buffer, sz);
}


int SSL_read(SSL* ssl, void* buffer, int sz)
{
    Data data(min(sz, MAX_RECORD_SIZE), static_cast<opaque*>(buffer));
    return receiveData(*ssl, data);
}


int SSL_accept(SSL* ssl)
{
    if (ssl->GetError() == YasslError(SSL_ERROR_WANT_READ))
        ssl->SetError(no_error);

    switch (ssl->getStates().GetAccept()) {

    case ACCEPT_BEGIN :
        processReply(*ssl);
        if (!ssl->GetError())
            ssl->useStates().UseAccept() = ACCEPT_FIRST_REPLY_DONE;

    case ACCEPT_FIRST_REPLY_DONE :
        sendServerHello(*ssl);

        if (!ssl->getSecurity().get_resuming()) {
            sendCertificate(*ssl);

            if (ssl->getSecurity().get_connection().send_server_key_)
                sendServerKeyExchange(*ssl);

            if(ssl->getCrypto().get_certManager().verifyPeer())
                sendCertificateRequest(*ssl);

            sendServerHelloDone(*ssl);
            ssl->flushBuffer();
        }
      
        if (!ssl->GetError())
            ssl->useStates().UseAccept() = SERVER_HELLO_DONE;

    case SERVER_HELLO_DONE :
        if (!ssl->getSecurity().get_resuming()) {
            while (ssl->getStates().getServer() < clientFinishedComplete) {
                if (ssl->GetError()) break;
                processReply(*ssl);
            }
        }
        if (!ssl->GetError())
            ssl->useStates().UseAccept() = ACCEPT_SECOND_REPLY_DONE;

    case ACCEPT_SECOND_REPLY_DONE :
        sendChangeCipher(*ssl);
        sendFinished(*ssl, server_end);
        ssl->flushBuffer();

        if (!ssl->GetError())
            ssl->useStates().UseAccept() = ACCEPT_FINISHED_DONE;

    case ACCEPT_FINISHED_DONE :
        if (ssl->getSecurity().get_resuming()) {
            while (ssl->getStates().getServer() < clientFinishedComplete) {
                if (ssl->GetError()) break;
                processReply(*ssl);
            }
        }
        if (!ssl->GetError())
            ssl->useStates().UseAccept() = ACCEPT_THIRD_REPLY_DONE;

    case ACCEPT_THIRD_REPLY_DONE :
        ssl->useLog().ShowTCP(ssl->getSocket().get_fd());

        if (ssl->GetError()) {
            GetErrors().Add(ssl->GetError());
            return SSL_FATAL_ERROR;
        }
        return SSL_SUCCESS;

    default:
        return SSL_FATAL_ERROR; // unknown state
    }
}


int SSL_do_handshake(SSL* ssl)
{
    if (ssl->getSecurity().get_parms().entity_ == client_end)
        return SSL_connect(ssl);
    else
        return SSL_accept(ssl);
}


int SSL_clear(SSL* ssl)
{
    GetErrors().Remove();

    return SSL_SUCCESS;
}


int SSL_shutdown(SSL* ssl)
{
    if (!ssl->GetQuietShutdown()) {
      Alert alert(warning, close_notify);
      sendAlert(*ssl, alert);
    }
    ssl->useLog().ShowTCP(ssl->getSocket().get_fd(), true);

    GetErrors().Remove();

    return SSL_SUCCESS;
}


void SSL_set_quiet_shutdown(SSL *ssl,int mode)
{
    ssl->SetQuietShutdown(mode != 0);
}


int SSL_get_quiet_shutdown(SSL *ssl)
{
    return ssl->GetQuietShutdown();
}


/* on by default but allow user to turn off */
long SSL_CTX_set_session_cache_mode(SSL_CTX* ctx, long mode)
{
    if (mode == SSL_SESS_CACHE_OFF)
        ctx->SetSessionCacheOff();

    if (mode == SSL_SESS_CACHE_NO_AUTO_CLEAR)
        ctx->SetSessionCacheFlushOff();

    return SSL_SUCCESS;
}


SSL_SESSION* SSL_get_session(SSL* ssl)
{
    if (ssl->getSecurity().GetContext()->GetSessionCacheOff())
        return 0;

    return GetSessions().lookup(
        ssl->getSecurity().get_connection().sessionID_);
}


int SSL_set_session(SSL* ssl, SSL_SESSION* session)
{
    if (ssl->getSecurity().GetContext()->GetSessionCacheOff())
        return SSL_FAILURE;

    ssl->set_session(session);
    return SSL_SUCCESS;
}


int SSL_session_reused(SSL* ssl)
{
    return ssl->getSecurity().get_resuming();
}


long SSL_SESSION_set_timeout(SSL_SESSION* sess, long t)
{
    if (!sess)
        return SSL_ERROR_NONE;

    sess->SetTimeOut(t);
    return SSL_SUCCESS;
}


long SSL_get_default_timeout(SSL* /*ssl*/)
{
    return DEFAULT_TIMEOUT;
}


void SSL_flush_sessions(SSL_CTX *ctx, long /* tm */)
{
    if (ctx->GetSessionCacheOff())
        return;

    GetSessions().Flush();
}


const char* SSL_get_cipher_name(SSL* ssl)
{ 
    return SSL_get_cipher(ssl); 
}


const char* SSL_get_cipher(SSL* ssl)
{
    return ssl->getSecurity().get_parms().cipher_name_;
}


// SSLv2 only, not implemented
char* SSL_get_shared_ciphers(SSL* /*ssl*/, char* buf, int len)
{
    return strncpy(buf, "Not Implemented, SSLv2 only", len);
}


const char* SSL_get_cipher_list(SSL* ssl, int priority)
{
    if (priority < 0 || priority >= MAX_CIPHERS)
        return 0;

    if (ssl->getSecurity().get_parms().cipher_list_[priority][0])
        return ssl->getSecurity().get_parms().cipher_list_[priority];

    return 0;
}


int SSL_CTX_set_cipher_list(SSL_CTX* ctx, const char* list)
{
    if (ctx->SetCipherList(list))
        return SSL_SUCCESS;
    else
        return SSL_FAILURE;
}


const char* SSL_get_version(SSL* ssl)
{
    static const char* version3 =  "SSLv3";
    static const char* version31 = "TLSv1";

    return ssl->isTLS() ? version31 : version3;
}

const char* SSLeay_version(int)
{
    static const char* version = "SSLeay yaSSL compatibility";
    return version;
}


int SSL_get_error(SSL* ssl, int /*previous*/)
{
    return ssl->getStates().What();
}



/* turn on yaSSL zlib compression
   returns 0 for success, else error (not built in)
   only need to turn on for client, becuase server on by default if built in
   but calling for server will tell you whether it's available or not
*/
int yaSSL_set_compression(SSL* ssl)
{
    return ssl->SetCompression();
}



X509* SSL_get_peer_certificate(SSL* ssl)
{
    return ssl->getCrypto().get_certManager().get_peerX509();
}


void X509_free(X509* /*x*/)
{
    // peer cert set for deletion during destruction
    // no need to delete now
}


X509* X509_STORE_CTX_get_current_cert(X509_STORE_CTX* ctx)
{
    return ctx->current_cert;
}


int X509_STORE_CTX_get_error(X509_STORE_CTX* ctx)
{
    return ctx->error;
}


int X509_STORE_CTX_get_error_depth(X509_STORE_CTX* ctx)
{
    return ctx->error_depth;
}


// copy name into buffer, at most sz bytes, if buffer is null
// will malloc buffer, caller responsible for freeing
char* X509_NAME_oneline(X509_NAME* name, char* buffer, int sz)
{
    if (!name->GetName()) return buffer;

    int len    = (int)strlen(name->GetName()) + 1;
    int copySz = min(len, sz);

    if (!buffer) {
        buffer = (char*)malloc(len);
        if (!buffer) return buffer;
        copySz = len;
    }

    if (copySz == 0)
        return buffer;

    memcpy(buffer, name->GetName(), copySz - 1);
    buffer[copySz - 1] = 0;

    return buffer;
}


X509_NAME* X509_get_issuer_name(X509* x)
{
    return  x->GetIssuer();
}


X509_NAME* X509_get_subject_name(X509* x)
{
    return x->GetSubject();
}


void SSL_load_error_strings()   // compatibility only 
{}


void SSL_set_connect_state(SSL*)
{
    // already a client by default
}


void SSL_set_accept_state(SSL* ssl)
{
    ssl->useSecurity().use_parms().entity_ = server_end;
}


long SSL_get_verify_result(SSL*)
{
    // won't get here if not OK
    return X509_V_OK;
}


long SSL_CTX_sess_set_cache_size(SSL_CTX* /*ctx*/, long /*sz*/)
{
    // unlimited size, can't set for now
    return 0;
}


long SSL_CTX_get_session_cache_mode(SSL_CTX*)
{
    // always 0, unlimited size for now
    return 0;
}


long SSL_CTX_set_tmp_dh(SSL_CTX* ctx, DH* dh)
{
    if (ctx->SetDH(*dh))
        return SSL_SUCCESS;
    else
        return SSL_FAILURE;
}


int SSL_CTX_use_certificate_file(SSL_CTX* ctx, const char* file, int format)
{
    return read_file(ctx, file, format, Cert);
}


int SSL_CTX_use_PrivateKey_file(SSL_CTX* ctx, const char* file, int format)
{
    return read_file(ctx, file, format, PrivateKey);
}


void SSL_CTX_set_verify(SSL_CTX* ctx, int mode, VerifyCallback vc)
{
    if (mode & SSL_VERIFY_PEER)
        ctx->setVerifyPeer();

    if (mode == SSL_VERIFY_NONE)
        ctx->setVerifyNone();

    if (mode & SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
        ctx->setFailNoCert();

    ctx->setVerifyCallback(vc);
}


int SSL_CTX_load_verify_locations(SSL_CTX* ctx, const char* file,
                                  const char* path)
{
    int       ret = SSL_SUCCESS;
    const int HALF_PATH = 128;

    if (file) ret = read_file(ctx, file, SSL_FILETYPE_PEM, CA);

    if (ret == SSL_SUCCESS && path) {
        // call read_file for each reqular file in path
#ifdef _WIN32

        WIN32_FIND_DATA FindFileData;
        HANDLE hFind;

        char name[MAX_PATH + 1];  // directory specification
        strncpy(name, path, MAX_PATH - 3);
        strncat(name, "\\*", 3);

        hFind = FindFirstFile(name, &FindFileData);
        if (hFind == INVALID_HANDLE_VALUE) return SSL_BAD_PATH;

        do {
            if (FindFileData.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
                strncpy(name, path, MAX_PATH - 2 - HALF_PATH);
                strncat(name, "\\", 2);
                strncat(name, FindFileData.cFileName, HALF_PATH);
                ret = read_file(ctx, name, SSL_FILETYPE_PEM, CA);
            }
        } while (ret == SSL_SUCCESS && FindNextFile(hFind, &FindFileData));

        FindClose(hFind);

#else   // _WIN32

        const int MAX_PATH = 260;

        DIR* dir = opendir(path);
        if (!dir) return SSL_BAD_PATH;

        struct dirent* entry;
        struct stat    buf;
        char           name[MAX_PATH + 1];

        while (ret == SSL_SUCCESS && (entry = readdir(dir))) {
            strncpy(name, path, MAX_PATH - 1 - HALF_PATH);
            strncat(name, "/", 1);
            strncat(name, entry->d_name, HALF_PATH);
            if (stat(name, &buf) < 0) return SSL_BAD_STAT;
     
            if (S_ISREG(buf.st_mode))
                ret = read_file(ctx, name, SSL_FILETYPE_PEM, CA);
        }

        closedir(dir);

#endif
    }

    return ret;
}


int SSL_CTX_set_default_verify_paths(SSL_CTX* /*ctx*/)
{
    // TODO: figure out way to set/store default path, then call load_verify
    return SSL_NOT_IMPLEMENTED;
}


int SSL_CTX_set_session_id_context(SSL_CTX*, const unsigned char*,
                                    unsigned int)
{
    // No application specific context needed for yaSSL
    return SSL_SUCCESS;
}


int SSL_CTX_check_private_key(SSL_CTX* /*ctx*/)
{
    // TODO: check private against public for RSA match
    return SSL_NOT_IMPLEMENTED;
}


// TODO: all session stats
long SSL_CTX_sess_accept(SSL_CTX* ctx)
{
    return ctx->GetStats().accept_;
}


long SSL_CTX_sess_connect(SSL_CTX* ctx)
{
    return ctx->GetStats().connect_;
}


long SSL_CTX_sess_accept_good(SSL_CTX* ctx)
{
    return ctx->GetStats().acceptGood_;
}


long SSL_CTX_sess_connect_good(SSL_CTX* ctx)
{
    return ctx->GetStats().connectGood_;
}


long SSL_CTX_sess_accept_renegotiate(SSL_CTX* ctx)
{
    return ctx->GetStats().acceptRenegotiate_;
}


long SSL_CTX_sess_connect_renegotiate(SSL_CTX* ctx)
{
    return ctx->GetStats().connectRenegotiate_;
}


long SSL_CTX_sess_hits(SSL_CTX* ctx)
{
    return ctx->GetStats().hits_;
}


long SSL_CTX_sess_cb_hits(SSL_CTX* ctx)
{
    return ctx->GetStats().cbHits_;
}


long SSL_CTX_sess_cache_full(SSL_CTX* ctx)
{
    return ctx->GetStats().cacheFull_;
}


long SSL_CTX_sess_misses(SSL_CTX* ctx)
{
    return ctx->GetStats().misses_;
}


long SSL_CTX_sess_timeouts(SSL_CTX* ctx)
{
    return ctx->GetStats().timeouts_;
}


long SSL_CTX_sess_number(SSL_CTX* ctx)
{
    return ctx->GetStats().number_;
}


long SSL_CTX_sess_get_cache_size(SSL_CTX* ctx)
{
    return ctx->GetStats().getCacheSize_;
}
// end session stats TODO:


int SSL_CTX_get_verify_mode(SSL_CTX* ctx)
{
    return ctx->GetStats().verifyMode_;
}


int SSL_get_verify_mode(SSL* ssl)
{
    return ssl->getSecurity().GetContext()->GetStats().verifyMode_;
}


int SSL_CTX_get_verify_depth(SSL_CTX* ctx)
{
    return ctx->GetStats().verifyDepth_;
}


int SSL_get_verify_depth(SSL* ssl)
{
    return ssl->getSecurity().GetContext()->GetStats().verifyDepth_;
}


long SSL_CTX_set_options(SSL_CTX*, long)
{
    // TDOD:
    return SSL_SUCCESS;
}


void SSL_CTX_set_info_callback(SSL_CTX*, void (*)())
{
    // TDOD:
}


void OpenSSL_add_all_algorithms()  // compatibility only
{}


int SSL_library_init()  // compatiblity only
{
    return 1;
}


DH* DH_new(void)
{
    DH* dh = NEW_YS DH;
    if (dh)
        dh->p = dh->g = 0;
    return dh;
}


void DH_free(DH* dh)
{
    ysDelete(dh->g);
    ysDelete(dh->p);
    ysDelete(dh);
}


// convert positive big-endian num of length sz into retVal, which may need to 
// be created
BIGNUM* BN_bin2bn(const unsigned char* num, int sz, BIGNUM* retVal)
{
    bool created = false;
    mySTL::auto_ptr<BIGNUM> bn;

    if (!retVal) {
        created = true;
        bn.reset(NEW_YS BIGNUM);
        retVal = bn.get();
    }

    retVal->assign(num, sz);

    if (created)
        return bn.release();
    else
        return retVal;
}


unsigned long ERR_get_error_line_data(const char**, int*, const char**, int *)
{
    //return SSL_NOT_IMPLEMENTED;
    return 0;
}


void ERR_print_errors_fp(FILE* /*fp*/)
{
    // need ssl access to implement TODO:
    //fprintf(fp, "%s", ssl.get_states().errorString_.c_str());
}


char* ERR_error_string(unsigned long errNumber, char* buffer)
{
  static char* msg = (char*)"Please supply a buffer for error string";

    if (buffer) {
        SetErrorString(YasslError(errNumber), buffer);
        return buffer;
    }

    return msg;
}


const char* X509_verify_cert_error_string(long /* error */)
{
    // TODO:
    static const char* msg = "Not Implemented";
    return msg;
}


const EVP_MD* EVP_md5(void)
{
    static const char* type = "MD5";
    return type;
}


const EVP_CIPHER* EVP_des_ede3_cbc(void)
{
    static const char* type = "DES-EDE3-CBC";
    return type;
}


int EVP_BytesToKey(const EVP_CIPHER* type, const EVP_MD* md, const byte* salt,
                   const byte* data, int sz, int count, byte* key, byte* iv)
{
    // only support MD5 for now
    if (strncmp(md, "MD5", 3)) return 0;

    int keyLen = 0;
    int ivLen  = 0;

    // only support CBC DES and AES for now
    if (strncmp(type, "DES-CBC", 7) == 0) {
        keyLen = DES_KEY_SZ;
        ivLen  = DES_IV_SZ;
    }
    else if (strncmp(type, "DES-EDE3-CBC", 12) == 0) {
        keyLen = DES_EDE_KEY_SZ;
        ivLen  = DES_IV_SZ;
    }
    else if (strncmp(type, "AES-128-CBC", 11) == 0) {
        keyLen = AES_128_KEY_SZ;
        ivLen  = AES_IV_SZ;
    }
    else if (strncmp(type, "AES-192-CBC", 11) == 0) {
        keyLen = AES_192_KEY_SZ;
        ivLen  = AES_IV_SZ;
    }
    else if (strncmp(type, "AES-256-CBC", 11) == 0) {
        keyLen = AES_256_KEY_SZ;
        ivLen  = AES_IV_SZ;
    }
    else
        return 0;

    yaSSL::MD5 myMD;
    uint digestSz = myMD.get_digestSize();
    byte digest[SHA_LEN];                   // max size

    int keyLeft   = keyLen;
    int ivLeft    = ivLen;
    int keyOutput = 0;

    while (keyOutput < (keyLen + ivLen)) {
        int digestLeft = digestSz;
        // D_(i - 1)
        if (keyOutput)                      // first time D_0 is empty
            myMD.update(digest, digestSz);
        // data
        myMD.update(data, sz);
        // salt
        if (salt)
            myMD.update(salt, EVP_SALT_SZ);
        myMD.get_digest(digest);
        // count
        for (int j = 1; j < count; j++) {
            myMD.update(digest, digestSz);
            myMD.get_digest(digest);
        }

        if (keyLeft) {
            int store = min(keyLeft, static_cast<int>(digestSz));
            memcpy(&key[keyLen - keyLeft], digest, store);

            keyOutput  += store;
            keyLeft    -= store;
            digestLeft -= store;
        }

        if (ivLeft && digestLeft) {
            int store = min(ivLeft, digestLeft);
            memcpy(&iv[ivLen - ivLeft], &digest[digestSz - digestLeft], store);

            keyOutput += store;
            ivLeft    -= store;
        }
    }
    assert(keyOutput == (keyLen + ivLen));
    return keyOutput;
}



void DES_set_key_unchecked(const_DES_cblock* key, DES_key_schedule* schedule)
{
    memcpy(schedule, key, sizeof(const_DES_cblock));
}


void DES_ede3_cbc_encrypt(const byte* input, byte* output, long sz,
                          DES_key_schedule* ks1, DES_key_schedule* ks2,
                          DES_key_schedule* ks3, DES_cblock* ivec, int enc)
{
    DES_EDE des;
    byte key[DES_EDE_KEY_SZ];

    memcpy(key, *ks1, DES_BLOCK);
    memcpy(&key[DES_BLOCK], *ks2, DES_BLOCK);
    memcpy(&key[DES_BLOCK * 2], *ks3, DES_BLOCK);

    if (enc) {
        des.set_encryptKey(key, *ivec);
        des.encrypt(output, input, sz);
    }
    else {
        des.set_decryptKey(key, *ivec);
        des.decrypt(output, input, sz);
    }
}


// functions for libcurl
int RAND_status()
{
    return 1;  /* TaoCrypt provides enough seed */
}


int DES_set_key(const_DES_cblock* key, DES_key_schedule* schedule)
{
    memcpy(schedule, key, sizeof(const_DES_cblock));
    return 1;
}


void DES_set_odd_parity(DES_cblock* key)
{
    // not needed now for TaoCrypt
}


void DES_ecb_encrypt(DES_cblock* input, DES_cblock* output,
                     DES_key_schedule* key, int enc)
{
    DES  des;

    if (enc) {
        des.set_encryptKey(*key, 0);
        des.encrypt(*output, *input, DES_BLOCK);
    }
    else {
        des.set_decryptKey(*key, 0);
        des.decrypt(*output, *input, DES_BLOCK);
    }
}


void SSL_CTX_set_default_passwd_cb_userdata(SSL_CTX* ctx, void* userdata)
{
    ctx->SetUserData(userdata);
}


X509* SSL_get_certificate(SSL* ssl)
{
    // only used to pass to get_privatekey which isn't used
    return 0;
}


EVP_PKEY* SSL_get_privatekey(SSL* ssl)
{
    // only called, not used
    return 0;
}


void SSL_SESSION_free(SSL_SESSION* session)
{
    // managed by singleton
}



EVP_PKEY* X509_get_pubkey(X509* x)
{
    // called, not used though
    return 0;
}


int EVP_PKEY_copy_parameters(EVP_PKEY* to, const EVP_PKEY* from)
{
    // called, not used though
    return 0;
}


void EVP_PKEY_free(EVP_PKEY* pkey)
{
    // never allocated from above
}


void ERR_error_string_n(unsigned long e, char *buf, size_t len)
{
    if (len) ERR_error_string(e, buf);
}


void ERR_free_strings(void)
{
    // handled internally
}


void EVP_cleanup(void)
{
    // nothing to do yet
}


ASN1_TIME* X509_get_notBefore(X509* x)
{
    if (x) return x->GetBefore();
    return 0;
}


ASN1_TIME* X509_get_notAfter(X509* x)
{
    if (x) return x->GetAfter();
    return 0;
}


SSL_METHOD* SSLv2_client_method(void)   /* will never work, no v 2    */
{
    return 0;
}


SSL_SESSION* SSL_get1_session(SSL* ssl)  /* what's ref count */
{
    return SSL_get_session(ssl);
}


void GENERAL_NAMES_free(STACK_OF(GENERAL_NAME) *x)
{
    // no extension names supported yet
}


int sk_GENERAL_NAME_num(STACK_OF(GENERAL_NAME) *x)
{
    // no extension names supported yet
    return 0;
}


GENERAL_NAME* sk_GENERAL_NAME_value(STACK_OF(GENERAL_NAME) *x, int i)
{
    // no extension names supported yet
    return 0;
}


unsigned char* ASN1_STRING_data(ASN1_STRING* x)
{
    if (x) return x->data;
    return 0;
}


int ASN1_STRING_length(ASN1_STRING* x)
{
    if (x) return x->length;
    return 0;
}


int ASN1_STRING_type(ASN1_STRING *x)
{
    if (x) return x->type;
    return 0;
}


int X509_NAME_get_index_by_NID(X509_NAME* name,int nid, int lastpos)
{
    int idx = -1;  // not found
    const char* start = &name->GetName()[lastpos + 1];

    switch (nid) {
    case NID_commonName:
        const char* found = strstr(start, "/CN=");
        if (found) {
            found += 4;  // advance to str
            idx = found - start + lastpos + 1;
        }
        break;
    }

    return idx;
}


ASN1_STRING* X509_NAME_ENTRY_get_data(X509_NAME_ENTRY* ne)
{
    // the same in yaSSL
    return ne;
}


X509_NAME_ENTRY* X509_NAME_get_entry(X509_NAME* name, int loc)
{
    return name->GetEntry(loc);
}


// already formatted, caller responsible for freeing *out
int ASN1_STRING_to_UTF8(unsigned char** out, ASN1_STRING* in)
{
    if (!in) return 0;

    *out = (unsigned char*)malloc(in->length + 1);
    if (*out) {
        memcpy(*out, in->data, in->length);
        (*out)[in->length] = 0;
    }
    return in->length;
}


void* X509_get_ext_d2i(X509* x, int nid, int* crit, int* idx)
{
    // no extensions supported yet
    return 0;
}


void MD4_Init(MD4_CTX* md4)
{
    // make sure we have a big enough buffer
    typedef char ok[sizeof(md4->buffer) >= sizeof(TaoCrypt::MD4) ? 1 : -1];
    (void) sizeof(ok);

    // using TaoCrypt since no dynamic memory allocated
    // and no destructor will be called
    new (reinterpret_cast<yassl_pointer>(md4->buffer)) TaoCrypt::MD4();
}


void MD4_Update(MD4_CTX* md4, const void* data, unsigned long sz)
{
    reinterpret_cast<TaoCrypt::MD4*>(md4->buffer)->Update(
                static_cast<const byte*>(data), static_cast<unsigned int>(sz));
}


void MD4_Final(unsigned char* hash, MD4_CTX* md4)
{
    reinterpret_cast<TaoCrypt::MD4*>(md4->buffer)->Final(hash);
}


void MD5_Init(MD5_CTX* md5)
{
    // make sure we have a big enough buffer
    typedef char ok[sizeof(md5->buffer) >= sizeof(TaoCrypt::MD5) ? 1 : -1];
    (void) sizeof(ok);

    // using TaoCrypt since no dynamic memory allocated
    // and no destructor will be called
    new (reinterpret_cast<yassl_pointer>(md5->buffer)) TaoCrypt::MD5();
}


void MD5_Update(MD5_CTX* md5, const void* data, unsigned long sz)
{
    reinterpret_cast<TaoCrypt::MD5*>(md5->buffer)->Update(
                static_cast<const byte*>(data), static_cast<unsigned int>(sz));
}


void MD5_Final(unsigned char* hash, MD5_CTX* md5)
{
    reinterpret_cast<TaoCrypt::MD5*>(md5->buffer)->Final(hash);
}


int RAND_bytes(unsigned char* buf, int num)
{
    RandomPool ran;

    if (ran.GetError()) return 0;

    ran.Fill(buf, num);
    return 1;
}


int SSL_peek(SSL* ssl, void* buffer, int sz)
{
    Data data(min(sz, MAX_RECORD_SIZE), static_cast<opaque*>(buffer));
    return receiveData(*ssl, data, true);
}


int SSL_pending(SSL* ssl)
{
    // Just in case there's pending data that hasn't been processed yet...
    char c;
    SSL_peek(ssl, &c, 1);
    
    return ssl->bufferedData();
}


void SSL_CTX_set_default_passwd_cb(SSL_CTX* ctx, pem_password_cb cb)
{
    ctx->SetPasswordCb(cb);
}


int SSLeay_add_ssl_algorithms()  // compatibility only
{
    return 1;
}


void ERR_remove_state(unsigned long)
{
    GetErrors().Remove();
}


int ERR_GET_REASON(int l)
{
    return l & 0xfff;
}


unsigned long err_helper(bool peek = false)
{
    int ysError = GetErrors().Lookup(peek);

    // translate cert error for libcurl, it uses OpenSSL hex code
    switch (ysError) {
    case TaoCrypt::SIG_OTHER_E:
        return CERTFICATE_ERROR;
        break;
    default :
        return 0;
    }

    return 0;  // shut up compiler
}


unsigned long ERR_peek_error()
{
    return err_helper(true);
}


unsigned long ERR_get_error()
{
    return err_helper();
}


    // functions for stunnel

    void RAND_screen()
    {
        // TODO:
    }


    const char* RAND_file_name(char*, size_t)
    {
        // TODO:
        return 0;
    }


    int RAND_write_file(const char*)
    {
        // TODO:
        return 0;
    }


    int RAND_load_file(const char*, long)
    {
        // TODO:
        return 0;
    }


    void RSA_free(RSA*)
    {
        // TODO:
    }


    RSA* RSA_generate_key(int, unsigned long, void(*)(int, int, void*), void*)
    {
        //  TODO:
        return 0;
    }


    int X509_LOOKUP_add_dir(X509_LOOKUP*, const char*, long)
    {
        // TODO:
        return SSL_SUCCESS;
    }


    int X509_LOOKUP_load_file(X509_LOOKUP*, const char*, long)
    {
        // TODO:
        return SSL_SUCCESS;
    }


    X509_LOOKUP_METHOD* X509_LOOKUP_hash_dir(void)
    {
        // TODO:
        return 0;
    }


    X509_LOOKUP_METHOD* X509_LOOKUP_file(void)
    {
        // TODO:
        return 0;
    }


    X509_LOOKUP* X509_STORE_add_lookup(X509_STORE*, X509_LOOKUP_METHOD*)
    {
        // TODO:
        return 0;
    }


    int X509_STORE_get_by_subject(X509_STORE_CTX*, int, X509_NAME*, X509_OBJECT*)
    {
        // TODO:
        return SSL_SUCCESS;
    }


    X509_STORE* X509_STORE_new(void)
    {
        // TODO:
        return 0;
    }

    char* SSL_alert_type_string_long(int)
    {
        // TODO:
        return 0;
    }


    char* SSL_alert_desc_string_long(int)
    {
        // TODO:
        return 0;
    }


    char* SSL_state_string_long(SSL*)
    {
        // TODO:
        return 0;
    }


    void SSL_CTX_set_tmp_rsa_callback(SSL_CTX*, RSA*(*)(SSL*, int, int))
    {
        // TDOD:
    }


    long SSL_CTX_set_timeout(SSL_CTX*, long)
    {
        // TDOD:
        return SSL_SUCCESS;
    }


    int SSL_CTX_use_certificate_chain_file(SSL_CTX*, const char*)
    {
        // TDOD:
        return SSL_SUCCESS;
    }


    int SSL_CTX_use_RSAPrivateKey_file(SSL_CTX*, const char*, int)
    {
        // TDOD:
        return SSL_SUCCESS;
    }


    int SSL_set_rfd(SSL*, int)
    {
        return SSL_SUCCESS; // TODO:
    }


    int SSL_set_wfd(SSL*, int)
    {
        return SSL_SUCCESS; // TODO:
    }


    int SSL_want_read(SSL*)
    {
        return 0; // TODO:
    }


    int SSL_want_write(SSL*)
    {
        return 0; // TODO:
    }


    void SSL_set_shutdown(SSL*, int)
    {
        // TODO:
    }


    SSL_CIPHER* SSL_get_current_cipher(SSL*)
    {
        // TODO:
        return 0;
    }


    char* SSL_CIPHER_description(SSL_CIPHER*, char*, int)
    {
        // TODO:
        return 0;
    }



    // end stunnel needs


} // extern "C"
} // namespace

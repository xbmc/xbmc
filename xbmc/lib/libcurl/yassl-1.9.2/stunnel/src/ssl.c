/*
 *   stunnel       Universal SSL tunnel
 *   Copyright (c) 1998-2004 Michal Trojnara <Michal.Trojnara@mirt.net>
 *                 All Rights Reserved
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   In addition, as a special exception, Michal Trojnara gives
 *   permission to link the code of this program with the OpenSSL
 *   library (or with modified versions of OpenSSL that use the same
 *   license as OpenSSL), and distribute linked combinations including
 *   the two.  You must obey the GNU General Public License in all
 *   respects for all of the code used other than OpenSSL.  If you modify
 *   this file, you may extend this exception to your version of the
 *   file, but you are not obligated to do so.  If you do not wish to
 *   do so, delete this exception statement from your version.
 */
#ifdef __vms
#include <starlet.h>
#endif /* __vms */

#ifndef NO_RSA

/* Cache temporary keys up to 2048 bits */
#define KEY_CACHE_LENGTH 2049

/* Cache temporary keys up to 1 hour */
#define KEY_CACHE_TIME 3600

#endif /* NO_RSA */

#include "common.h"
#include "prototypes.h"

    /* SSL functions */
static int init_dh(void);
static int init_prng(void);
static int prng_seeded(int);
static int add_rand_file(char *);
#ifndef NO_RSA
static RSA *tmp_rsa_cb(SSL *, int, int);
static RSA *make_temp_key(int);
#endif /* NO_RSA */
static void verify_init(void);
static int verify_callback(int, X509_STORE_CTX *);
static int crl_callback(X509_STORE_CTX *);
#if SSLEAY_VERSION_NUMBER >= 0x00907000L
static void info_callback(const SSL *, int, int);
#else
static void info_callback(SSL *, int, int);
#endif
static void print_stats(void);
static void sslerror_stack(void);

SSL_CTX *ctx; /* global SSL context */
static X509_STORE *revocation_store=NULL;

void context_init(void) { /* init SSL */
    int i;

#if SSLEAY_VERSION_NUMBER >= 0x00907000L
    /* Load all bundled ENGINEs into memory and make them visible */
    ENGINE_load_builtin_engines();
    /* Register all of them for every algorithm they collectively implement */
    ENGINE_register_all_complete();
#endif
    if(!init_prng())
        log(LOG_INFO, "PRNG seeded successfully");
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
    if(options.option.client) {
        ctx=SSL_CTX_new(SSLv3_client_method());
    } else { /* Server mode */
        ctx=SSL_CTX_new(SSLv23_server_method());
#ifndef NO_RSA
        SSL_CTX_set_tmp_rsa_callback(ctx, tmp_rsa_cb);
#endif /* NO_RSA */
        if(init_dh())
            log(LOG_WARNING, "Diffie-Hellman initialization failed");
    }
    if(options.ssl_options) {
        log(LOG_DEBUG, "Configuration SSL options: 0x%08lX",
            options.ssl_options);
        log(LOG_DEBUG, "SSL options set: 0x%08lX", 
            SSL_CTX_set_options(ctx, options.ssl_options));
    }
#if SSLEAY_VERSION_NUMBER >= 0x00906000L
    SSL_CTX_set_mode(ctx,
        SSL_MODE_ENABLE_PARTIAL_WRITE|SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
#endif /* OpenSSL-0.9.6 */

    SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_BOTH);
    SSL_CTX_set_timeout(ctx, options.session_timeout);
    if(options.option.cert) {
        if(!SSL_CTX_use_certificate_chain_file(ctx, options.cert)) {
            log(LOG_ERR, "Error reading certificate file: %s", options.cert);
            sslerror("SSL_CTX_use_certificate_chain_file");
            exit(1);
        }
        log(LOG_DEBUG, "Certificate: %s", options.cert);
        log(LOG_DEBUG, "Key file: %s", options.key);
#ifdef USE_WIN32
        SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_cb);
#endif
        for(i=0; i<3; i++) {
#ifdef NO_RSA
            if(SSL_CTX_use_PrivateKey_file(ctx, options.key,
                    SSL_FILETYPE_PEM))
#else /* NO_RSA */
            if(SSL_CTX_use_RSAPrivateKey_file(ctx, options.key,
                    SSL_FILETYPE_PEM))
#endif /* NO_RSA */
                break;
            if(i<2 && ERR_GET_REASON(ERR_peek_error())==EVP_R_BAD_DECRYPT) {
                sslerror_stack(); /* dump the error stack */
                log(LOG_ERR, "Wrong pass phrase: retrying");
                continue;
            }
#ifdef NO_RSA
            sslerror("SSL_CTX_use_PrivateKey_file");
#else /* NO_RSA */
            sslerror("SSL_CTX_use_RSAPrivateKey_file");
#endif /* NO_RSA */
            exit(1);
        }
        if(!SSL_CTX_check_private_key(ctx)) {
            sslerror("Private key does not match the certificate");
            exit(1);
        }
    }

    verify_init(); /* Initialize certificate verification */

    SSL_CTX_set_info_callback(ctx, info_callback);

    if(options.cipher_list) {
        if (!SSL_CTX_set_cipher_list(ctx, options.cipher_list)) {
            sslerror("SSL_CTX_set_cipher_list");
            exit(1);
        }
    }
}

void context_free(void) { /* free SSL */
    SSL_CTX_free(ctx);
}

static int init_prng(void) {
    int totbytes=0;
    char filename[STRLEN];
    int bytes;
    
    bytes=0; /* avoid warning if #ifdef'd out for windows */

    filename[0]='\0';

    /* If they specify a rand file on the command line we
       assume that they really do want it, so try it first */
    if(options.rand_file) {
        totbytes+=add_rand_file(options.rand_file);
        if(prng_seeded(totbytes))
            return 0;
    }

    /* try the $RANDFILE or $HOME/.rnd files */
    RAND_file_name(filename, STRLEN);
    if(filename[0]) {
        filename[STRLEN-1]='\0';        /* just in case */
        totbytes+=add_rand_file(filename);
        if(prng_seeded(totbytes))
            return 0;
    }

#ifdef RANDOM_FILE
    totbytes += add_rand_file( RANDOM_FILE );
    if(prng_seeded(totbytes))
        return 0;
#endif

#ifdef USE_WIN32
    RAND_screen();
    if(prng_seeded(totbytes)) {
        log(LOG_DEBUG, "Seeded PRNG with RAND_screen");
        return 0;
    }
    log(LOG_DEBUG, "RAND_screen failed to sufficiently seed PRNG");
#else

#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
    if(options.egd_sock) {
        if((bytes=RAND_egd(options.egd_sock))==-1) {
            log(LOG_WARNING, "EGD Socket %s failed", options.egd_sock);
            bytes=0;
        } else {
            totbytes += bytes;
            log(LOG_DEBUG, "Snagged %d random bytes from EGD Socket %s",
                bytes, options.egd_sock);
            return 0; /* OpenSSL always gets what it needs or fails,
                         so no need to check if seeded sufficiently */
        }
    }
#ifdef EGD_SOCKET
    if((bytes=RAND_egd(EGD_SOCKET))==-1) {
        log(LOG_WARNING, "EGD Socket %s failed", EGD_SOCKET);
    } else {
        totbytes += bytes;
        log(LOG_DEBUG, "Snagged %d random bytes from EGD Socket %s",
            bytes, EGD_SOCKET);
        return 0;
    }
#endif /* EGD_SOCKET */

#endif /* OpenSSL-0.9.5a */
#endif /* USE_WIN32 */

    /* Try the good-old default /dev/urandom, if available  */
    totbytes+=add_rand_file( "/dev/urandom" );
    if(prng_seeded(totbytes))
        return 0;

    /* Random file specified during configure */
    log(LOG_INFO, "PRNG seeded with %d bytes total", totbytes);
    log(LOG_WARNING, "PRNG may not have been seeded with enough random bytes");
    return -1; /* FAILED */
}

static int init_dh(void) {
#ifdef USE_DH
    FILE *fp;
    DH *dh;
    BIO *bio;

    fp=fopen(options.cert, "r");
    if(!fp) {
#ifdef USE_WIN32
        /* Win32 doesn't seem to set errno in fopen() */
        log(LOG_ERR, "Failed to open %s", options.cert);
#else
        ioerror(options.cert);
#endif
        return -1; /* FAILED */
    }
    bio=BIO_new_fp(fp, BIO_CLOSE|BIO_FP_TEXT);
    if(!bio) {
        log(LOG_ERR, "BIO_new_fp failed");
        return -1; /* FAILED */
    }
    if((dh=PEM_read_bio_DHparams(bio, NULL, NULL
#if SSLEAY_VERSION_NUMBER >= 0x00904000L
            , NULL
#endif
            ))) {
        BIO_free(bio);
        log(LOG_DEBUG, "Using Diffie-Hellman parameters from %s",
            options.cert);
    } else { /* Failed to load DH parameters from file */
        BIO_free(bio);
        log(LOG_NOTICE, "Could not load DH parameters from %s", options.cert);
        return -1; /* FAILED */
    }
    SSL_CTX_set_tmp_dh(ctx, dh);
    log(LOG_INFO, "Diffie-Hellman initialized with %d bit key",
        8*DH_size(dh));
    DH_free(dh);
#endif /* USE_DH */
    return 0; /* OK */
}

/* shortcut to determine if sufficient entropy for PRNG is present */
static int prng_seeded(int bytes) {
#if SSLEAY_VERSION_NUMBER >= 0x0090581fL
    if(RAND_status()){
        log(LOG_DEBUG, "RAND_status claims sufficient entropy for the PRNG");
        return 1;
    }
#else
    if(bytes>=options.random_bytes) {
        log(LOG_INFO, "Sufficient entropy in PRNG assumed (>= %d)", options.random_bytes);
        return 1;
    }
#endif
    return 0;        /* assume we don't have enough */
}

static int add_rand_file(char *filename) {
    int readbytes;
    int writebytes;
    struct stat sb;

    if(stat(filename, &sb))
        return 0;
    if((readbytes=RAND_load_file(filename, options.random_bytes)))
        log(LOG_DEBUG, "Snagged %d random bytes from %s", readbytes, filename);
    else
        log(LOG_INFO, "Unable to retrieve any random data from %s", filename);
    /* Write new random data for future seeding if it's a regular file */
    if(options.option.rand_write && (sb.st_mode & S_IFREG)){
        writebytes = RAND_write_file(filename);
        if(writebytes==-1)
            log(LOG_WARNING, "Failed to write strong random data to %s - "
                "may be a permissions or seeding problem", filename);
        else
            log(LOG_DEBUG, "Wrote %d new random bytes to %s", writebytes, filename);
    }
    return readbytes;
}

#ifndef NO_RSA

static RSA *tmp_rsa_cb(SSL *s, int export, int keylen) {
    static int initialized=0;
    static struct keytabstruct {
        RSA *key;
        time_t timeout;
    } keytable[KEY_CACHE_LENGTH];
    static RSA *longkey=NULL;
    static int longlen=0;
    static time_t longtime=0;
    RSA *oldkey, *retval;
    time_t now;
    int i;

    enter_critical_section(CRIT_KEYGEN); /* Only one make_temp_key() at a time */
    if(!initialized) {
        for(i=0; i<KEY_CACHE_LENGTH; i++) {
            keytable[i].key=NULL;
            keytable[i].timeout=0;
        }
        initialized=1;
    }
    time(&now);
    if(keylen<KEY_CACHE_LENGTH) {
        if(keytable[keylen].timeout<now) {
            oldkey=keytable[keylen].key;
            keytable[keylen].key=make_temp_key(keylen);
            keytable[keylen].timeout=now+KEY_CACHE_TIME;
            if(oldkey)
                RSA_free(oldkey);
        }
        retval=keytable[keylen].key;
    } else { /* Temp key > 2048 bits.  Is it possible? */
        if(longtime<now || longlen!=keylen) {
            oldkey=longkey;
            longkey=make_temp_key(keylen);
            longtime=now+KEY_CACHE_TIME;
            longlen=keylen;
            if(oldkey)
                RSA_free(oldkey);
        }
        retval=longkey;
    }
    leave_critical_section(CRIT_KEYGEN);
    return retval;
}

static RSA *make_temp_key(int keylen) {
    RSA *result;

    log(LOG_DEBUG, "Generating %d bit temporary RSA key...", keylen);
#if SSLEAY_VERSION_NUMBER >= 0x0900
    result=RSA_generate_key(keylen, RSA_F4, NULL, NULL);
#else
    result=RSA_generate_key(keylen, RSA_F4, NULL);
#endif
    log(LOG_DEBUG, "Temporary RSA key created");
    return result;
}

#endif /* NO_RSA */

static void verify_init(void) {
    X509_LOOKUP *lookup;

    if(options.verify_level<0)
        return; /* No certificate verification */

    if(options.verify_level>1 && !options.ca_file && !options.ca_dir) {
        log(LOG_ERR, "Either CApath or CAfile "
            "has to be used for authentication");
        exit(1);
    }

    if(options.ca_file) {
        if(!SSL_CTX_load_verify_locations(ctx, options.ca_file, NULL)) {
            log(LOG_ERR, "Error loading verify certificates from %s",
                options.ca_file);
            sslerror("SSL_CTX_load_verify_locations");
            exit(1);
        }
#if 0
        SSL_CTX_set_client_CA_list(ctx,
            SSL_load_client_CA_file(options.ca_file));
#endif
        log(LOG_DEBUG, "Loaded verify certificates from %s",
            options.ca_file);
    }

    if(options.ca_dir) {
        if(!SSL_CTX_load_verify_locations(ctx, NULL, options.ca_dir)) {
            log(LOG_ERR, "Error setting verify directory to %s",
                options.ca_dir);
            sslerror("SSL_CTX_load_verify_locations");
            exit(1);
        }
        log(LOG_DEBUG, "Verify directory set to %s", options.ca_dir);
    }

    if(options.crl_file || options.crl_dir) { /* setup CRL store */
        revocation_store=X509_STORE_new();
        if(!revocation_store) {
            sslerror("X509_STORE_new");
            exit(1);
        }
        if(options.crl_file) {
            lookup=X509_STORE_add_lookup(revocation_store,
                X509_LOOKUP_file());
            if(!lookup) {
                sslerror("X509_STORE_add_lookup");
                exit(1);
            }
            if(!X509_LOOKUP_load_file(lookup, options.crl_file,
                    X509_FILETYPE_PEM)) {
                log(LOG_ERR, "Error loading CRLs from %s",
                    options.crl_file);
                sslerror("X509_LOOKUP_load_file");
                exit(1);
            }
            log(LOG_DEBUG, "Loaded CRLs from %s", options.crl_file);
        }
        if(options.crl_dir) {
            lookup=X509_STORE_add_lookup(revocation_store,
                X509_LOOKUP_hash_dir());
            if(!lookup) {
                sslerror("X509_STORE_add_lookup");
                exit(1);
            }
            if(!X509_LOOKUP_add_dir(lookup, options.crl_dir,
                    X509_FILETYPE_PEM)) {
                log(LOG_ERR, "Error setting CRL directory to %s",
                    options.crl_dir);
                sslerror("X509_LOOKUP_add_dir");
                exit(1);
            }
            log(LOG_DEBUG, "CRL directory set to %s", options.crl_dir);
        }
    }

    SSL_CTX_set_verify(ctx, options.verify_level==SSL_VERIFY_NONE ?
        SSL_VERIFY_PEER : options.verify_level, verify_callback);

    if(options.ca_dir && options.verify_use_only_my)
        log(LOG_NOTICE, "Peer certificate location %s", options.ca_dir);
}

static int verify_callback(int preverify_ok, X509_STORE_CTX *callback_ctx) {
        /* our verify callback function */
    char txt[STRLEN];
    X509_OBJECT ret;

    X509_NAME_oneline(X509_get_subject_name(callback_ctx->current_cert),
        txt, STRLEN);
    safestring(txt);
    if(options.verify_level==SSL_VERIFY_NONE) {
        log(LOG_NOTICE, "VERIFY IGNORE: depth=%d, %s",
            callback_ctx->error_depth, txt);
        return 1; /* Accept connection */
    }
    if(!preverify_ok) {
        /* Remote site specified a certificate, but it's not correct */
        log(LOG_WARNING, "VERIFY ERROR: depth=%d, error=%s: %s",
            callback_ctx->error_depth,
            X509_verify_cert_error_string (callback_ctx->error), txt);
        return 0; /* Reject connection */
    }
    if(options.verify_use_only_my && callback_ctx->error_depth==0 &&
            X509_STORE_get_by_subject(callback_ctx, X509_LU_X509,
                X509_get_subject_name(callback_ctx->current_cert), &ret)!=1) {
        log(LOG_WARNING, "VERIFY ERROR ONLY MY: no cert for %s", txt);
        return 0; /* Reject connection */
    }
    if(revocation_store && !crl_callback(callback_ctx))
        return 0; /* Reject connection */
    /* errnum = X509_STORE_CTX_get_error(ctx); */

    log(LOG_NOTICE, "VERIFY OK: depth=%d, %s", callback_ctx->error_depth, txt);
    return 1; /* Accept connection */
}

/* Based on BSD-style licensed code of mod_ssl */
static int crl_callback(X509_STORE_CTX *callback_ctx) {
#ifndef HAVE_YASSL /* yassl add, no rev list support yet */
    X509_STORE_CTX store_ctx;
    X509_OBJECT obj;
    X509_NAME *subject;
    X509_NAME *issuer;
    X509 *xs;
    X509_CRL *crl;
    X509_REVOKED *revoked;
    EVP_PKEY *pubkey;
    long serial;
    BIO *bio;
    int i, n, rc;
    char *cp;
    char *cp2;
    ASN1_TIME *t;

    /* Determine certificate ingredients in advance */
    xs      = X509_STORE_CTX_get_current_cert(callback_ctx);
    subject = X509_get_subject_name(xs);
    issuer  = X509_get_issuer_name(xs);

    /* Try to retrieve a CRL corresponding to the _subject_ of
     * the current certificate in order to verify it's integrity. */
    memset((char *)&obj, 0, sizeof(obj));
    X509_STORE_CTX_init(&store_ctx, revocation_store, NULL, NULL);
    rc=X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, subject, &obj);
    X509_STORE_CTX_cleanup(&store_ctx);
    crl=obj.data.crl;
    if(rc>0 && crl) {
        /* Log information about CRL
         * (A little bit complicated because of ASN.1 and BIOs...) */
        bio=BIO_new(BIO_s_mem());
        BIO_printf(bio, "lastUpdate: ");
        ASN1_UTCTIME_print(bio, X509_CRL_get_lastUpdate(crl));
        BIO_printf(bio, ", nextUpdate: ");
        ASN1_UTCTIME_print(bio, X509_CRL_get_nextUpdate(crl));
        n=BIO_pending(bio);
        cp=malloc(n+1);
        n=BIO_read(bio, cp, n);
        cp[n]='\0';
        BIO_free(bio);
        cp2=X509_NAME_oneline(subject, NULL, 0);
        log(LOG_NOTICE, "CA CRL: Issuer: %s, %s", cp2, cp);
        OPENSSL_free(cp2);
        free(cp);

        /* Verify the signature on this CRL */
        pubkey=X509_get_pubkey(xs);
        if(X509_CRL_verify(crl, pubkey)<=0) {
            log(LOG_WARNING, "Invalid signature on CRL");
            X509_STORE_CTX_set_error(callback_ctx,
                X509_V_ERR_CRL_SIGNATURE_FAILURE);
            X509_OBJECT_free_contents(&obj);
            if(pubkey)
                EVP_PKEY_free(pubkey);
            return 0; /* Reject connection */
        }
        if(pubkey)
            EVP_PKEY_free(pubkey);

        /* Check date of CRL to make sure it's not expired */
        t=X509_CRL_get_nextUpdate(crl);
        if(!t) {
            log(LOG_WARNING, "Found CRL has invalid nextUpdate field");
            X509_STORE_CTX_set_error(callback_ctx,
                X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD);
            X509_OBJECT_free_contents(&obj);
            return 0; /* Reject connection */
        }
        if(X509_cmp_current_time(t)<0) {
            log(LOG_WARNING, "Found CRL is expired - "
                "revoking all certificates until you get updated CRL");
            X509_STORE_CTX_set_error(callback_ctx, X509_V_ERR_CRL_HAS_EXPIRED);
            X509_OBJECT_free_contents(&obj);
            return 0; /* Reject connection */
        }
        X509_OBJECT_free_contents(&obj);
    }

    /* Try to retrieve a CRL corresponding to the _issuer_ of
     * the current certificate in order to check for revocation. */
    memset((char *)&obj, 0, sizeof(obj));
    X509_STORE_CTX_init(&store_ctx, revocation_store, NULL, NULL);
    rc=X509_STORE_get_by_subject(&store_ctx, X509_LU_CRL, issuer, &obj);
    X509_STORE_CTX_cleanup(&store_ctx);
    crl=obj.data.crl;
    if(rc>0 && crl) {
        /* Check if the current certificate is revoked by this CRL */
#if SSL_LIBRARY_VERSION < 0x00904000
        n=sk_num(X509_CRL_get_REVOKED(crl));
#else
        n=sk_X509_REVOKED_num(X509_CRL_get_REVOKED(crl));
#endif
        for(i=0; i<n; i++) {
#if SSL_LIBRARY_VERSION < 0x00904000
            revoked=(X509_REVOKED *)sk_value(X509_CRL_get_REVOKED(crl), i);
#else
            revoked=sk_X509_REVOKED_value(X509_CRL_get_REVOKED(crl), i);
#endif
            if(ASN1_INTEGER_cmp(revoked->serialNumber,
                    X509_get_serialNumber(xs)) == 0) {
                serial=ASN1_INTEGER_get(revoked->serialNumber);
                cp=X509_NAME_oneline(issuer, NULL, 0);
                log(LOG_NOTICE, "Certificate with serial %ld (0x%lX) "
                    "revoked per CRL from issuer %s", serial, serial, cp);
                OPENSSL_free(cp);
                X509_STORE_CTX_set_error(callback_ctx, X509_V_ERR_CERT_REVOKED);
                X509_OBJECT_free_contents(&obj);
                return 0; /* Reject connection */
            }
        }
        X509_OBJECT_free_contents(&obj);
    }
#endif /* yassl add end */
    return 1; /* Accept connection */
}

#if SSLEAY_VERSION_NUMBER >= 0x00907000L
static void info_callback(const SSL *s, int where, int ret) {
#else
static void info_callback(SSL *s, int where, int ret) {
#endif
    if(where & SSL_CB_LOOP)
        log(LOG_DEBUG, "SSL state (%s): %s",
        where & SSL_ST_CONNECT ? "connect" :
        where & SSL_ST_ACCEPT ? "accept" :
        "undefined", SSL_state_string_long(s));
    else if(where & SSL_CB_ALERT)
        log(LOG_DEBUG, "SSL alert (%s): %s: %s",
            where & SSL_CB_READ ? "read" : "write",
            SSL_alert_type_string_long(ret),
            SSL_alert_desc_string_long(ret));
    else if(where==SSL_CB_HANDSHAKE_DONE)
        print_stats();
}

static void print_stats(void) { /* print statistics */
    log(LOG_DEBUG, "%4ld items in the session cache",
        SSL_CTX_sess_number(ctx));
    log(LOG_DEBUG, "%4ld client connects (SSL_connect())",
        SSL_CTX_sess_connect(ctx));
    log(LOG_DEBUG, "%4ld client connects that finished",
        SSL_CTX_sess_connect_good(ctx));
#if SSLEAY_VERSION_NUMBER >= 0x0922
    log(LOG_DEBUG, "%4ld client renegotiatations requested",
        SSL_CTX_sess_connect_renegotiate(ctx));
#endif
    log(LOG_DEBUG, "%4ld server connects (SSL_accept())",
        SSL_CTX_sess_accept(ctx));
    log(LOG_DEBUG, "%4ld server connects that finished",
        SSL_CTX_sess_accept_good(ctx));
#if SSLEAY_VERSION_NUMBER >= 0x0922
    log(LOG_DEBUG, "%4ld server renegotiatiations requested",
        SSL_CTX_sess_accept_renegotiate(ctx));
#endif
    log(LOG_DEBUG, "%4ld session cache hits", SSL_CTX_sess_hits(ctx));
    log(LOG_DEBUG, "%4ld session cache misses", SSL_CTX_sess_misses(ctx));
    log(LOG_DEBUG, "%4ld session cache timeouts", SSL_CTX_sess_timeouts(ctx));
}

void sslerror(char *txt) { /* SSL Error handler */
    unsigned long err;
    char string[120];

    err=ERR_get_error();
    if(!err) {
        log(LOG_ERR, "%s: Peer suddenly disconnected", txt);
        return;
    }
    sslerror_stack();
    ERR_error_string(err, string);
    log(LOG_ERR, "%s: %lX: %s", txt, err, string);
}

static void sslerror_stack(void) { /* recursive dump of the error stack */
    unsigned long err;
    char string[120];

    err=ERR_get_error();
    if(!err)
        return;
    sslerror_stack();
    ERR_error_string(err, string);
    log(LOG_ERR, "error stack: %lX : %s", err, string);
}

/* End of ssl.c */

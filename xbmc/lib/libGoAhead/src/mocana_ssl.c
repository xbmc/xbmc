/*
 * mocana_ssl.c
 *
 * Mocana Embedded SSL Server
 *   GoAhead integration layer
 *
 * Copyright Mocana Corp 2003. All Rights Reserved.
 *
 */


/******************************** Description *********************************/

/*
 *    This module implements a patch into the Mocana Embedded SSL Server.
 *
 */

#ifdef __ENABLE_MOCANA_SSL_SERVER__

/********************************* Includes ***********************************/

#include "mocana_ssl/common/moptions.h"
#include "mocana_ssl/common/mdefs.h"
#include "mocana_ssl/common/mtypes.h"
#include "mocana_ssl/common/merrors.h"
#include "mocana_ssl/common/mrtos.h"
#include "mocana_ssl/common/mtcp.h"
#include "mocana_ssl/common/mocana.h"
#include "mocana_ssl/common/random.h"
#include "mocana_ssl/ssl/ssl.h"

#include    "wsIntrn.h"
#include    "webs.h"
#include    "websSSL.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/******************************* Definitions **********************************/

#ifdef __RTOS_VXWORKS__
#define SSL_CERTIFICATE_DER_FILE        "NVRAM:/ssl.der"
#define SSL_RSA_HOST_KEYS               "NVRAM:/sslkey.dat"
#else
#define SSL_CERTIFICATE_DER_FILE        "ssl.der"
#define SSL_RSA_HOST_KEYS               "sslkey.dat"
#endif

#define SSL_PORT                        SSL_DEFAULT_TCPIP_PORT
#define MAX_SSL_CONNECTIONS_ALLOWED     (10)
#define SSL_HELLO_TIMEOUT               (15000)
#define SSL_RECV_TIMEOUT                (300000)

#define SSL_EXAMPLE_KEY_SIZE            (1024 + 256)

static int      sslListenSock   = -1;            /* Listen socket */
static char*    pCertificate    = NULL;
static char*    pRsaKeyBlob     = NULL;


/******************************* Prototypes  **********************************/

int         websSSLAccept(int sid, char *ipaddr, int port, int listenSid);
static void websSSLSocketEvent(int sid, int mask, int iwp);
static int  websSSLReadEvent(webs_t wp);
static int  mocana_SSL_computeHostKeys(char** ppCertificate, unsigned int *pCertLength,
                                       char** ppRsaKeyBlob,  unsigned int *pKeyBlobLength);
static int  mocana_SSL_releaseHostKeys(char **ppCertificate, char **ppRsaKeyBlob);



/******************************************************************************/
/*
 *    Start up the SSL Context for the application, and start a listen on the
 *    SSL port (usually 443, and defined by SSL_PORT)
 *    Return 0 on success, -1 on failure.
 */

int websSSLOpen()
{
    unsigned int certLength;
    unsigned int rsaKeyBlobLength;

    if (0 > MOCANA_initMocana())
        return -1;

    if (0 > mocana_SSL_computeHostKeys(&pCertificate, &certLength, &pRsaKeyBlob, &rsaKeyBlobLength))
        return -1;

    if (0 > SSL_init(pCertificate, certLength, pRsaKeyBlob, rsaKeyBlobLength, MAX_SSL_CONNECTIONS_ALLOWED))
        return -1;

    SSL_sslSettings()->sslTimeOutHello      = SSL_HELLO_TIMEOUT;
    SSL_sslSettings()->sslTimeOutReceive    = SSL_RECV_TIMEOUT;

    sslListenSock = socketOpenConnection(NULL, SSL_PORT, websSSLAccept, SOCKET_BLOCK);

    if (sslListenSock < 0) {
        trace(2, T("SSL: Unable to open SSL socket on port <%d>!\n"),
            SSL_PORT);
        return -1;
    }

    return 0;
}


/******************************************************************************/
/*
 *    Accept a connection
 */

int websSSLAccept(int sid, char *ipaddr, int port, int listenSid)
{
    webs_t    wp;
    int        wid;

    a_assert(ipaddr && *ipaddr);
    a_assert(sid >= 0);
    a_assert(port >= 0);

/*
 *    Allocate a new handle for this accepted connection. This will allocate
 *    a webs_t structure in the webs[] list
 */
    if ((wid = websAlloc(sid)) < 0) {
        return -1;
    }
    wp = webs[wid];
    a_assert(wp);
    wp->listenSid = listenSid;

    ascToUni(wp->ipaddr, ipaddr, min(sizeof(wp->ipaddr), strlen(ipaddr)+1));

/*
 *    Check if this is a request from a browser on this system. This is useful
 *    to know for permitting administrative operations only for local access
 */
    if (gstrcmp(wp->ipaddr, T("127.0.0.1")) == 0 ||
            gstrcmp(wp->ipaddr, websIpaddr) == 0 ||
            gstrcmp(wp->ipaddr, websHost) == 0) {
        wp->flags |= WEBS_LOCAL_REQUEST;
    }
/*
 *    Since the acceptance came in on this channel, it must be secure
 */
    wp->flags |= WEBS_SECURE;

/*
 *    Arrange for websSocketEvent to be called when read data is available
 */
    socketCreateHandler(sid, SOCKET_READABLE, websSSLSocketEvent, (int) wp);

/*
 *    Arrange for a timeout to kill hung requests
 */
    wp->timeout = emfSchedCallback(WEBS_TIMEOUT, websTimeout, (void *) wp);
    trace(8, T("webs: accept request\n"));
    return 0;
}


/******************************************************************************/
/*
 *    Perform a read of the SSL socket
 */

int websSSLRead(websSSL_t *wsp, char_t *buf, int len)
{
    int     numBytesReceived;
    int     rc;

    a_assert(wsp);
    a_assert(buf);

    if ((rc = SSL_recv(wsp->mocanaConnectionInstance, buf, len, &numBytesReceived)) < 0)
    {
        if (0 > rc)
            rc = -1;

        return rc;
    }

    return numBytesReceived;
}


/******************************************************************************/
/*
 *    The webs socket handler.  Called in response to I/O. We just pass control
 *    to the relevant read or write handler. A pointer to the webs structure
 *    is passed as an (int) in iwp.
 */

static void websSSLSocketEvent(int sid, int mask, int iwp)
{
    webs_t    wp;

    wp = (webs_t) iwp;
    a_assert(wp);

    if (! websValid(wp)) {
        return;
    }

    if (mask & SOCKET_READABLE) {
        websSSLReadEvent(wp);
    }
    if (mask & SOCKET_WRITABLE) {
        if (wp->writeSocket) {
            (*wp->writeSocket)(wp);
        }
    }
}


/******************************************************************************/
/*
 *    Handler for SSL Read Events
 */

static int websSSLReadEvent (webs_t wp)
{
    int            ret = 07, sock;
    socket_t    *sptr;
    int         connectionInstance;

    a_assert (wp);
    a_assert(websValid(wp));

    sptr = socketPtr(wp->sid);
    a_assert(sptr);

    sock = sptr->sock;

/*
 *    Create a new SSL session for this web request
 */

    connectionInstance = SSL_acceptConnection(sock);

    if (0 > connectionInstance)
    {
#ifdef __ENABLE_ALL_DEBUGGING__
        printf("websSSLReadEvent: SSL_acceptConnection failed. %d\n", connectionInstance);
#endif

        /* SSL error: cleanup */
        websTimeoutCancel(wp);
        socketCloseConnection(wp->sid);
        websFree(wp);

        return -1;
    }

/*
 *    Create the SSL data structure in the wp.
 */
    wp->wsp = balloc(B_L, sizeof(websSSL_t));
    a_assert (wp->wsp);

    (wp->wsp)->mocanaConnectionInstance = connectionInstance;
    (wp->wsp)->wp                       = wp;

/*
 *    Call the default Read Event
 */
    websReadEvent(wp);

    return ret;
}


/******************************************************************************/
/*
 *    Return TRUE if websSSL has been opened
 */

int websSSLIsOpen()
{
    return (sslListenSock != -1);
}


/******************************************************************************/
/*
 *    Perform a gets of the SSL socket, returning an balloc'ed string
 *
 *    Get a string from a socket. This returns data in *buf in a malloced string
 *    after trimming the '\n'. If there is zero bytes returned, *buf will be set
 *    to NULL. If doing non-blocking I/O, it returns -1 for error, EOF or when
 *    no complete line yet read. If doing blocking I/O, it will block until an
 *    entire line is read. If a partial line is read socketInputBuffered or
 *    socketEof can be used to distinguish between EOF and partial line still
 *    buffered. This routine eats and ignores carriage returns.
 */

int    websSSLGets(websSSL_t *wsp, char_t **buf)
{
    socket_t    *sp;
    ringq_t        *lq;
    char        c;
    int            rc, len;
    webs_t      wp;
    int         sid;
    int         mci;
    int         numBytesReceived;

    a_assert(wsp);
    a_assert(buf);

    *buf = NULL;

    wp  = wsp->wp;
    sid = wp->sid;
    mci = wsp->mocanaConnectionInstance;

    if ((sp = socketPtr(sid)) == NULL) {
        return -1;
    }
    lq = &sp->lineBuf;

    while (1) {

        if ((rc = SSL_recv(mci, &c, 1, &numBytesReceived)) < 0)
        {
            if (0 > rc)
                rc = -1;

            return rc;
        }

        if (numBytesReceived == 0) {
/*
 *            If there is a partial line and we are at EOF, pretend we saw a '\n'
 */
            if (ringqLen(lq) > 0 && (sp->flags & SOCKET_EOF)) {
                c = '\n';
            } else {
                continue;
            }
        }
/*
 *        If a newline is seen, return the data excluding the new line to the
 *        caller. If carriage return is seen, just eat it.
 */
        if (c == '\n') {
            len = ringqLen(lq);
            if (len > 0) {
                *buf = ballocAscToUni((char *)lq->servp, len);
            } else {
                *buf = NULL;
            }
            ringqFlush(lq);
            return len;

        } else if (c == '\r') {
            continue;
        }
        ringqPutcA(lq, c);
    }
    return 0;
}


/******************************************************************************/
/*
 *    Perform a write to the SSL socket
 */

int websSSLWrite(websSSL_t *wsp, char_t *buf, int len)
{
    int sslBytesSent;

    a_assert(wsp);
    a_assert(buf);

    if (wsp == NULL) {
        return -1;
    }

    sslBytesSent = SSL_send(wsp->mocanaConnectionInstance, buf, len);

    if (0 > sslBytesSent)
        sslBytesSent = -1;

    return sslBytesSent;
}


/******************************************************************************/
/*
 *    Return Eof for the underlying socket
 */

int websSSLEof(websSSL_t *wsp)
{
    webs_t      wp;
    int         sid;

    a_assert(wsp);

    wp  = wsp->wp;
    sid = wp->sid;

    return socketEof(sid);
}


/******************************************************************************/
/*
 *   Flush stub for compatibility
 */
int websSSLFlush(websSSL_t *wsp)
{
    a_assert(wsp);

    /* Autoflush - do nothing */
    return 0;
}


/******************************************************************************/
/*
 *    Free SSL resources
 */

int websSSLFree(websSSL_t *wsp)
{
    int status;
    if (NULL != wsp)
    {
        int mci;

        mci = wsp->mocanaConnectionInstance;

        status = SSL_closeConnection(mci);

        if (0 > status)
            status = -1;

        /* Free memory here.... */
        bfree(B_L, wsp);
    }

    return status;
}


/******************************************************************************/
/*
 *    Stops the SSL system
 */

void websSSLClose()
{
    SSL_shutdown();
    mocana_SSL_releaseHostKeys(&pCertificate, &pRsaKeyBlob);
    SSL_releaseTables();
    MOCANA_freeMocana();
}


/******************************************************************************/

static int
mocana_SSL_testHostKeys(char** ppCertificate, unsigned int *pCertLength,
                         char** ppRsaKeyBlob,  unsigned int *pKeyBlobLength)
{
    int             status;

    if (0 > (status = MOCANA_readFile(SSL_CERTIFICATE_DER_FILE, ppCertificate, pCertLength)))
        goto exit;

    status = MOCANA_readFile(SSL_RSA_HOST_KEYS, ppRsaKeyBlob, pKeyBlobLength);

exit:
    return status;
}


/******************************************************************************/

static int
mocana_SSL_releaseHostKeys(char **ppCertificate, char **ppRsaKeyBlob)
{
    MOCANA_freeReadFile(ppCertificate);
    MOCANA_freeReadFile(ppRsaKeyBlob);

    return 0;
}


/******************************************************************************/

static int
mocana_SSL_computeHostKeys(char** ppCertificate, unsigned int *pCertLength,
                            char** ppRsaKeyBlob,  unsigned int *pKeyBlobLength)
{
    int   status;

    *ppCertificate = NULL;
    *ppRsaKeyBlob  = NULL;

    /* check for pre-existing set of host keys */
    if (0 > (status = mocana_SSL_testHostKeys(ppCertificate, pCertLength, ppRsaKeyBlob, pKeyBlobLength)))
    {
#ifdef __ENABLE_ALL_DEBUGGING__
        printf("mocana_SSL_computeHostKeys: host keys do not exist, computing new key pair.\n");
#endif

        /* if not, compute new host keys */
        if (0 > (status = SSL_generateCertificate(ppCertificate, pCertLength, ppRsaKeyBlob, pKeyBlobLength, SSL_EXAMPLE_KEY_SIZE)))
            goto exit;

        if (0 > (status = MOCANA_writeFile(SSL_CERTIFICATE_DER_FILE, *ppCertificate, *pCertLength)))
            goto exit;

        status = MOCANA_writeFile(SSL_RSA_HOST_KEYS, *ppRsaKeyBlob, *pKeyBlobLength);

#ifdef __ENABLE_ALL_DEBUGGING__
        printf("mocana_SSL_computeHostKeys: host key computation completed.\n");
#endif
    }

exit:
    if (0 > status)
        SSL_freeCertificate(ppCertificate, ppRsaKeyBlob);

    return status;
}


/******************************************************************************/

#endif

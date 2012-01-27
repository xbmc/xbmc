/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 *	from: @(#)svc.h 1.20 88/02/08 SMI 
 *	from: @(#)svc.h	2.2 88/07/29 4.0 RPCSRC
 *	$Id: svc.h,v 1.4 2004/11/25 19:41:19 emoy Exp $
 */

/*
 * svc.h, Server-side remote procedure call interface.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _RPC_SVC_H
#define _RPC_SVC_H
#include <sys/cdefs.h>

/*
 * This interface must manage two items concerning remote procedure calling:
 *
 * 1) An arbitrary number of transport connections upon which rpc requests
 * are received.  The two most notable transports are TCP and UDP;  they are
 * created and registered by routines in svc_tcp.c and svc_udp.c, respectively;
 * they in turn call xprt_register and xprt_unregister.
 *
 * 2) An arbitrary number of locally registered services.  Services are
 * described by the following four data: program number, version number,
 * "service dispatch" function, a transport handle, and a boolean that
 * indicates whether or not the exported program should be registered with a
 * local binder service;  if true the program's number and version and the
 * port number from the transport handle are registered with the binder.
 * These data are registered with the rpc svc system via svc_register.
 *
 * A service's dispatch function is called whenever an rpc request comes in
 * on a transport.  The request's program and version numbers must match
 * those of the registered service.  The dispatch function is passed two
 * parameters, struct svc_req * and SVCXPRT *, defined below.
 */

enum xprt_stat {
	XPRT_DIED,
	XPRT_MOREREQS,
	XPRT_IDLE
};

/*
 * Server side transport handle
 */
typedef struct {
	int		xp_sock;
	unsigned short		xp_port;	 /* associated port number */
	struct xp_ops {
#ifdef __cplusplus
	    bool_t	(*xp_recv)(...);	 /* receive incomming requests */
	    enum xprt_stat (*xp_stat)(...); /* get transport status */
	    bool_t	(*xp_getargs)(...); /* get arguments */
	    bool_t	(*xp_reply)(...);	 /* send reply */
	    bool_t	(*xp_freeargs)(...);/* free mem allocated for args */
	    void	(*xp_destroy)(...); /* destroy this struct */
#else
	/* DO NOT REMOVE THE COMMENTED OUT ...: fixincludes needs to see them */
	    bool_t	(*xp_recv)(/*...*/);	 /* receive incomming requests */
	    enum xprt_stat (*xp_stat)(/*...*/); /* get transport status */
	    bool_t	(*xp_getargs)(/*...*/); /* get arguments */
	    bool_t	(*xp_reply)(/*...*/);	 /* send reply */
	    bool_t	(*xp_freeargs)(/*...*/);/* free mem allocated for args */
	    void	(*xp_destroy)(/*...*/); /* destroy this struct */
#endif
	} *xp_ops;
	int		xp_addrlen;	 /* length of remote address */
	struct sockaddr_in xp_raddr;	 /* remote address */
	struct opaque_auth xp_verf;	 /* raw response verifier */
	caddr_t		xp_p1;		 /* private */
	caddr_t		xp_p2;		 /* private */
} SVCXPRT;

/*
 *  Approved way of getting address of caller
 */
#define svc_getcaller(x) (&(x)->xp_raddr)

/*
 * Operations defined on an SVCXPRT handle
 *
 * SVCXPRT		*xprt;
 * struct rpc_msg	*msg;
 * xdrproc_t		 xargs;
 * caddr_t		 argsp;
 */
#define SVC_RECV(xprt, msg)				\
	(*(xprt)->xp_ops->xp_recv)((xprt), (msg))
#define svc_recv(xprt, msg)				\
	(*(xprt)->xp_ops->xp_recv)((xprt), (msg))

#define SVC_STAT(xprt)					\
	(*(xprt)->xp_ops->xp_stat)(xprt)
#define svc_stat(xprt)					\
	(*(xprt)->xp_ops->xp_stat)(xprt)

#define SVC_GETARGS(xprt, xargs, argsp)			\
	(*(xprt)->xp_ops->xp_getargs)((xprt), (xargs), (argsp))
#define svc_getargs(xprt, xargs, argsp)			\
	(*(xprt)->xp_ops->xp_getargs)((xprt), (xargs), (argsp))

#define SVC_REPLY(xprt, msg)				\
	(*(xprt)->xp_ops->xp_reply) ((xprt), (msg))
#define svc_reply(xprt, msg)				\
	(*(xprt)->xp_ops->xp_reply) ((xprt), (msg))

#define SVC_FREEARGS(xprt, xargs, argsp)		\
	(*(xprt)->xp_ops->xp_freeargs)((xprt), (xargs), (argsp))
#define svc_freeargs(xprt, xargs, argsp)		\
	(*(xprt)->xp_ops->xp_freeargs)((xprt), (xargs), (argsp))

#define SVC_DESTROY(xprt)				\
	(*(xprt)->xp_ops->xp_destroy)(xprt)
#define svc_destroy(xprt)				\
	(*(xprt)->xp_ops->xp_destroy)(xprt)


/*
 * Service request
 */
struct svc_req {
#ifdef __LP64__
	unsigned int		rq_prog;	/* service program number */
	unsigned int		rq_vers;	/* service protocol version */
	unsigned int		rq_proc;	/* the desired procedure */
#else
	unsigned long		rq_prog;	/* service program number */
	unsigned long		rq_vers;	/* service protocol version */
	unsigned long		rq_proc;	/* the desired procedure */
#endif
	struct opaque_auth rq_cred;	/* raw creds from the wire */
	caddr_t		rq_clntcred;	/* read only cooked cred */
	SVCXPRT	*rq_xprt;		/* associated transport */
};


/*
 * Service registration
 *
 * svc_register(xprt, prog, vers, dispatch, protocol)
 *	SVCXPRT *xprt;
 *	u_long prog;
 *	u_long vers;
 *	void (*dispatch)(...); // fixincludes needs the ..., even in a comment
 *	int protocol;  like TCP or UDP, zero means do not register 
 */
__BEGIN_DECLS
#ifdef __LP64__
extern bool_t	svc_register __P((SVCXPRT *, unsigned int, unsigned int, void (*)(), int));
#else
extern bool_t	svc_register __P((SVCXPRT *, unsigned long, unsigned long, void (*)(), int));
#endif
__END_DECLS

/*
 * Service un-registration
 *
 * svc_unregister(prog, vers)
 *	u_long prog;
 *	u_long vers;
 */
__BEGIN_DECLS
#ifdef __LP64__
extern void	svc_unregister __P((unsigned int, unsigned int));
#else
extern void	svc_unregister __P((unsigned long, unsigned long));
#endif
__END_DECLS

/*
 * Transport registration.
 *
 * xprt_register(xprt)
 *	SVCXPRT *xprt;
 */
__BEGIN_DECLS
extern void	xprt_register	__P((SVCXPRT *));
__END_DECLS

/*
 * Transport un-register
 *
 * xprt_unregister(xprt)
 *	SVCXPRT *xprt;
 */
__BEGIN_DECLS
extern void	xprt_unregister	__P((SVCXPRT *));
__END_DECLS




/*
 * When the service routine is called, it must first check to see if it
 * knows about the procedure;  if not, it should call svcerr_noproc
 * and return.  If so, it should deserialize its arguments via 
 * SVC_GETARGS (defined above).  If the deserialization does not work,
 * svcerr_decode should be called followed by a return.  Successful
 * decoding of the arguments should be followed the execution of the
 * procedure's code and a call to svc_sendreply.
 *
 * Also, if the service refuses to execute the procedure due to too-
 * weak authentication parameters, svcerr_weakauth should be called.
 * Note: do not confuse access-control failure with weak authentication!
 *
 * NB: In pure implementations of rpc, the caller always waits for a reply
 * msg.  This message is sent when svc_sendreply is called.  
 * Therefore pure service implementations should always call
 * svc_sendreply even if the function logically returns void;  use
 * xdr.h - xdr_void for the xdr routine.  HOWEVER, tcp based rpc allows
 * for the abuse of pure rpc via batched calling or pipelining.  In the
 * case of a batched call, svc_sendreply should NOT be called since
 * this would send a return message, which is what batching tries to avoid.
 * It is the service/protocol writer's responsibility to know which calls are
 * batched and which are not.  Warning: responding to batch calls may
 * deadlock the caller and server processes!
 */

__BEGIN_DECLS
extern bool_t	svc_sendreply	__P((SVCXPRT *, xdrproc_t, char *));
extern void	svcerr_decode	__P((SVCXPRT *));
extern void	svcerr_weakauth	__P((SVCXPRT *));
extern void	svcerr_noproc	__P((SVCXPRT *));
#ifdef __LP64__
extern void	svcerr_progvers	__P((SVCXPRT *, unsigned int, unsigned int));
#else
extern void	svcerr_progvers	__P((SVCXPRT *, unsigned long, unsigned long));
#endif
extern void	svcerr_auth	__P((SVCXPRT *, enum auth_stat));
extern void	svcerr_noprog	__P((SVCXPRT *));
extern void	svcerr_systemerr __P((SVCXPRT *));
__END_DECLS
    
/*
 * Lowest level dispatching -OR- who owns this process anyway.
 * Somebody has to wait for incoming requests and then call the correct
 * service routine.  The routine svc_run does infinite waiting; i.e.,
 * svc_run never returns.
 * Since another (co-existant) package may wish to selectively wait for
 * incoming calls or other events outside of the rpc architecture, the
 * routine svc_getreq is provided.  It must be passed readfds, the
 * "in-place" results of a select system call (see select, section 2).
 */

/*
 * Global keeper of rpc service descriptors in use
 * dynamic; must be inspected before each call to select 
 */
#ifdef FD_SETSIZE
extern fd_set svc_fdset;
#define svc_fds svc_fdset.fds_bits[0]	/* compatibility */
#else
extern int svc_fds;
#endif /* def FD_SETSIZE */

/*
 * a small program implemented by the svc_rpc implementation itself;
 * also see clnt.h for protocol numbers.
 */
extern void rpctest_service();

__BEGIN_DECLS
extern void	svc_getreq	__P((int));
extern void	svc_getreqset	__P((fd_set *));
extern void	svc_run		__P((void));
__END_DECLS

/*
 * Socket to use on svcxxx_create call to get default socket
 */
#define	RPC_ANYSOCK	-1

/*
 * These are the existing service side transport implementations
 */

/*
 * Memory based rpc for testing and timing.
 */
__BEGIN_DECLS
extern SVCXPRT *svcraw_create __P((void));
__END_DECLS


/*
 * Udp based rpc.
 */
__BEGIN_DECLS
extern SVCXPRT *svcudp_create __P((int));
extern SVCXPRT *svcudp_bufcreate __P((int, unsigned int, unsigned int));
__END_DECLS


/*
 * Tcp based rpc.
 */
__BEGIN_DECLS
extern SVCXPRT *svctcp_create __P((int, unsigned int, unsigned int));
__END_DECLS

/*
 * Any open file descriptor based rpc.
 */
__BEGIN_DECLS
extern SVCXPRT *svcfd_create __P((int, u_int, u_int));
__END_DECLS

#endif /* !_RPC_SVC_H */

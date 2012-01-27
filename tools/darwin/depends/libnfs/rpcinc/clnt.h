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
 *	from: @(#)clnt.h 1.31 88/02/08 SMI
 *	from: @(#)clnt.h	2.1 88/07/29 4.0 RPCSRC
 *	$Id: clnt.h,v 1.4 2004/10/28 21:58:22 emoy Exp $
 */

/*
 * clnt.h - Client side remote procedure call interface.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _RPC_CLNT_H_
#define _RPC_CLNT_H_
#include <sys/cdefs.h>

/*
 * Rpc calls return an enum clnt_stat.  This should be looked at more,
 * since each implementation is required to live with this (implementation
 * independent) list of errors.
 */

/* Avoid collision with mach definition */
#if defined(RPC_SUCCESS)
#undef RPC_SUCCESS
#endif

enum clnt_stat {
	RPC_SUCCESS=0,			/* call succeeded */
	/*
	 * local errors
	 */
	RPC_CANTENCODEARGS=1,		/* can't encode arguments */
	RPC_CANTDECODERES=2,		/* can't decode results */
	RPC_CANTSEND=3,			/* failure in sending call */
	RPC_CANTRECV=4,			/* failure in receiving result */
	RPC_TIMEDOUT=5,			/* call timed out */
	/*
	 * remote errors
	 */
	RPC_VERSMISMATCH=6,		/* rpc versions not compatible */
	RPC_AUTHERROR=7,		/* authentication error */
	RPC_PROGUNAVAIL=8,		/* program not available */
	RPC_PROGVERSMISMATCH=9,		/* program version mismatched */
	RPC_PROCUNAVAIL=10,		/* procedure unavailable */
	RPC_CANTDECODEARGS=11,		/* decode arguments error */
	RPC_SYSTEMERROR=12,		/* generic "other problem" */

	/*
	 * callrpc & clnt_create errors
	 */
	RPC_UNKNOWNHOST=13,		/* unknown host name */
	RPC_UNKNOWNPROTO=17,		/* unkown protocol */

	/*
	 * _ create errors
	 */
	RPC_PMAPFAILURE=14,		/* the pmapper failed in its call */
	RPC_PROGNOTREGISTERED=15,	/* remote program is not registered */
	/*
	 * unspecified error
	 */
	RPC_FAILED=16
};


/*
 * Error info.
 */
struct rpc_err {
	enum clnt_stat re_status;
	union {
		int RE_errno;		/* realated system error */
		enum auth_stat RE_why;	/* why the auth error occurred */
		struct {
#ifdef __LP64__
			unsigned int low;	/* lowest verion supported */
			unsigned int high;	/* highest verion supported */
#else
			unsigned long low;	/* lowest verion supported */
			unsigned long high;	/* highest verion supported */
#endif
		} RE_vers;
		struct {		/* maybe meaningful if RPC_FAILED */
#ifdef __LP64__
			int s1;
			int s2;
#else
			long s1;
			long s2;
#endif
		} RE_lb;		/* life boot & debugging only */
	} ru;
#define	re_errno	ru.RE_errno
#define	re_why		ru.RE_why
#define	re_vers		ru.RE_vers
#define	re_lb		ru.RE_lb
};


/*
 * Client rpc handle.
 * Created by individual implementations, see e.g. rpc_udp.c.
 * Client is responsible for initializing auth, see e.g. auth_none.c.
 */
typedef struct CLIENT CLIENT;
struct CLIENT
{
	AUTH	*cl_auth;	/* authenticator */
	struct clnt_ops {
#ifdef __LP64__
		enum clnt_stat (*cl_call)(CLIENT *, unsigned int, xdrproc_t, void *, xdrproc_t, void *, struct timeval); 	/* call remote procedure */
#else
		enum clnt_stat (*cl_call)(CLIENT *, unsigned long, xdrproc_t, void *, xdrproc_t, void *, struct timeval); 	/* call remote procedure */
#endif
		void (*cl_abort)(void); 	/* abort a call */
		void (*cl_geterr)(CLIENT *, struct rpc_err *); 	/* get specific error code */
		bool_t (*cl_freeres)(CLIENT *, xdrproc_t, void *); 	/* frees results */
		void (*cl_destroy)(CLIENT *); 	/* destroy this structure */
		bool_t (*cl_control)(CLIENT *, int, char *); 	/* the ioctl() of rpc */
	} *cl_ops;
	caddr_t cl_private;	/* private stuff */
};

/*
 * client side rpc interface ops
 *
 * Parameter types are:
 *
 */

/*
 * enum clnt_stat
 * CLNT_CALL(rh, proc, xargs, argsp, xres, resp, timeout)
 * 	CLIENT *rh;
 *	u_long proc;
 *	xdrproc_t xargs;
 *	caddr_t argsp;
 *	xdrproc_t xres;
 *	caddr_t resp;
 *	struct timeval timeout;
 */
#define	CLNT_CALL(rh, proc, xargs, argsp, xres, resp, secs)	\
	((*(rh)->cl_ops->cl_call)(rh, proc, xargs, argsp, xres, resp, secs))
#define	clnt_call(rh, proc, xargs, argsp, xres, resp, secs)	\
	((*(rh)->cl_ops->cl_call)(rh, proc, xargs, argsp, xres, resp, secs))

/*
 * void
 * CLNT_ABORT(rh);
 * 	CLIENT *rh;
 */
#define	CLNT_ABORT(rh)	((*(rh)->cl_ops->cl_abort)(rh))
#define	clnt_abort(rh)	((*(rh)->cl_ops->cl_abort)(rh))

/*
 * struct rpc_err
 * CLNT_GETERR(rh);
 * 	CLIENT *rh;
 */
#define	CLNT_GETERR(rh,errp)	((*(rh)->cl_ops->cl_geterr)(rh, errp))
#define	clnt_geterr(rh,errp)	((*(rh)->cl_ops->cl_geterr)(rh, errp))


/*
 * bool_t
 * CLNT_FREERES(rh, xres, resp);
 * 	CLIENT *rh;
 *	xdrproc_t xres;
 *	caddr_t resp;
 */
#define	CLNT_FREERES(rh,xres,resp) ((*(rh)->cl_ops->cl_freeres)(rh,xres,resp))
#define	clnt_freeres(rh,xres,resp) ((*(rh)->cl_ops->cl_freeres)(rh,xres,resp))

/*
 * bool_t
 * CLNT_CONTROL(cl, request, info)
 *      CLIENT *cl;
 *      unsigned int request;
 *      char *info;
 */
#define	CLNT_CONTROL(cl,rq,in) ((*(cl)->cl_ops->cl_control)(cl,rq,in))
#define	clnt_control(cl,rq,in) ((*(cl)->cl_ops->cl_control)(cl,rq,in))

/*
 * control operations that apply to both udp and tcp transports
 */
#define CLSET_TIMEOUT       1   /* set timeout (timeval) */
#define CLGET_TIMEOUT       2   /* get timeout (timeval) */
#define CLGET_SERVER_ADDR   3   /* get server's address (sockaddr) */
/*
 * udp only control operations
 */
#define CLSET_RETRY_TIMEOUT 4   /* set retry timeout (timeval) */
#define CLGET_RETRY_TIMEOUT 5   /* get retry timeout (timeval) */

/*
 * void
 * CLNT_DESTROY(rh);
 * 	CLIENT *rh;
 */
#define	CLNT_DESTROY(rh)	((*(rh)->cl_ops->cl_destroy)(rh))
#define	clnt_destroy(rh)	((*(rh)->cl_ops->cl_destroy)(rh))


/*
 * RPCTEST is a test program which is accessable on every rpc
 * transport/port.  It is used for testing, performance evaluation,
 * and network administration.
 */

#ifdef __LP64__
#define RPCTEST_PROGRAM		((unsigned int)1)
#define RPCTEST_VERSION		((unsigned int)1)
#define RPCTEST_NULL_PROC	((unsigned int)2)
#define RPCTEST_NULL_BATCH_PROC	((unsigned int)3)
#else
#define RPCTEST_PROGRAM		((unsigned long)1)
#define RPCTEST_VERSION		((unsigned long)1)
#define RPCTEST_NULL_PROC	((unsigned long)2)
#define RPCTEST_NULL_BATCH_PROC	((unsigned long)3)
#endif

/*
 * By convention, procedure 0 takes null arguments and returns them
 */

#ifdef __LP64__
#define NULLPROC ((unsigned int)0)
#else
#define NULLPROC ((unsigned long)0)
#endif

/*
 * Below are the client handle creation routines for the various
 * implementations of client side rpc.  They can return NULL if a 
 * creation failure occurs.
 */

/*
 * Memory based rpc (for speed check and testing)
 * CLIENT *
 * clntraw_create(prog, vers)
 *	u_long prog;
 *	u_long vers;
 */
__BEGIN_DECLS
#ifdef __LP64__
extern CLIENT *clntraw_create	__P((unsigned int, unsigned int));
#else
extern CLIENT *clntraw_create	__P((unsigned long, unsigned long));
#endif
__END_DECLS


/*
 * Generic client creation routine. Supported protocols are "udp" and "tcp"
 * CLIENT *
 * clnt_create(host, prog, vers, prot);
 *	char *host; 	-- hostname
 *	u_long prog;	-- program number
 *	u_long vers;	-- version number
 *	char *prot;	-- protocol
 */
__BEGIN_DECLS
#ifdef __LP64__
extern CLIENT *clnt_create	__P((char *, unsigned int, unsigned int, char *));
#else
extern CLIENT *clnt_create	__P((char *, unsigned long, unsigned long, char *));
#endif
__END_DECLS


/*
 * TCP based rpc
 * CLIENT *
 * clnttcp_create(raddr, prog, vers, sockp, sendsz, recvsz)
 *	struct sockaddr_in *raddr;
 *	u_long prog;
 *	u_long version;
 *	register int *sockp;
 *	unsigned int sendsz;
 *	unsigned int recvsz;
 */
__BEGIN_DECLS
#ifdef __LP64__
extern CLIENT *clnttcp_create	__P((struct sockaddr_in *,
									 unsigned int,
									 unsigned int,
									 int *,
									 unsigned int,
									 unsigned int));
#else
extern CLIENT *clnttcp_create	__P((struct sockaddr_in *,
									 unsigned long,
									 unsigned long,
									 int *,
									 unsigned int,
									 unsigned int));
#endif
__END_DECLS


/*
 * UDP based rpc.
 * CLIENT *
 * clntudp_create(raddr, program, version, wait, sockp)
 *	struct sockaddr_in *raddr;
 *	u_long program;
 *	u_long version;
 *	struct timeval wait;
 *	int *sockp;
 *
 * Same as above, but you specify max packet sizes.
 * CLIENT *
 * clntudp_bufcreate(raddr, program, version, wait, sockp, sendsz, recvsz)
 *	struct sockaddr_in *raddr;
 *	u_long program;
 *	u_long version;
 *	struct timeval wait;
 *	int *sockp;
 *	unsigned int sendsz;
 *	unsigned int recvsz;
 */
__BEGIN_DECLS
#ifdef __LP64__
extern CLIENT *clntudp_create	__P((struct sockaddr_in *,
									 unsigned int,
									 unsigned int,
									 struct timeval,
									 int *));
extern CLIENT *clntudp_bufcreate __P((struct sockaddr_in *,
									  unsigned int,
									  unsigned int,
									  struct timeval,
									  int *,
									  unsigned int,
									  unsigned int));
#else
extern CLIENT *clntudp_create	__P((struct sockaddr_in *,
									 unsigned long,
									 unsigned long,
									 struct timeval,
									 int *));
extern CLIENT *clntudp_bufcreate __P((struct sockaddr_in *,
									  unsigned long,
									  unsigned long,
									  struct timeval,
									  int *,
									  unsigned int,
									  unsigned int));
#endif
__END_DECLS


/*
 * Print why creation failed
 */
__BEGIN_DECLS
extern void clnt_pcreateerror	__P((char *));			/* stderr */
extern char *clnt_spcreateerror	__P((char *));			/* string */
__END_DECLS

/*
 * Like clnt_perror(), but is more verbose in its output
 */ 
__BEGIN_DECLS
extern void clnt_perrno		__P((enum clnt_stat));		/* stderr */
extern char *clnt_sperrno	__P((enum clnt_stat));		/* string */
__END_DECLS

/*
 * Print an English error message, given the client error code
 */
__BEGIN_DECLS
extern void clnt_perror		__P((CLIENT *, char *)); 	/* stderr */
extern char *clnt_sperror	__P((CLIENT *, char *));	/* string */
__END_DECLS


/* 
 * If a creation fails, the following allows the user to figure out why.
 */
struct rpc_createerr {
	enum clnt_stat cf_stat;
	struct rpc_err cf_error; /* useful when cf_stat == RPC_PMAPFAILURE */
};

extern struct rpc_createerr rpc_createerr;


#define UDPMSGSIZE	8800	/* rpc imposed limit on udp msg size */
#define RPCSMALLMSGSIZE	400	/* a more reasonable packet size */

#endif /* !_RPC_CLNT_H */

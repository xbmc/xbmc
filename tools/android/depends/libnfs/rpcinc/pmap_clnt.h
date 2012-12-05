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
 *	from: @(#)pmap_clnt.h 1.11 88/02/08 SMI 
 *	from: @(#)pmap_clnt.h	2.1 88/07/29 4.0 RPCSRC
 *	$Id: pmap_clnt.h,v 1.3 2004/10/28 21:58:22 emoy Exp $
 */

/*
 * pmap_clnt.h
 * Supplies C routines to get to portmap services.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

/*
 * Usage:
 *	success = pmap_set(program, version, protocol, port);
 *	success = pmap_unset(program, version);
 *	port = pmap_getport(address, program, version, protocol);
 *	head = pmap_getmaps(address);
 *	clnt_stat = pmap_rmtcall(address, program, version, procedure,
 *		xdrargs, argsp, xdrres, resp, tout, port_ptr)
 *		(works for udp only.) 
 * 	clnt_stat = clnt_broadcast(program, version, procedure,
 *		xdrargs, argsp,	xdrres, resp, eachresult)
 *		(like pmap_rmtcall, except the call is broadcasted to all
 *		locally connected nets.  For each valid response received,
 *		the procedure eachresult is called.  Its form is:
 *	done = eachresult(resp, raddr)
 *		bool_t done;
 *		caddr_t resp;
 *		struct sockaddr_in raddr;
 *		where resp points to the results of the call and raddr is the
 *		address if the responder to the broadcast.
 */

#ifndef _RPC_PMAPCLNT_H
#define _RPC_PMAPCLNT_H
#include <sys/cdefs.h>

__BEGIN_DECLS
#ifdef __LP64__
extern bool_t		pmap_set	__P((unsigned int, unsigned int, int, int));
extern bool_t		pmap_unset	__P((unsigned int, unsigned int));
extern enum clnt_stat	pmap_rmtcall	__P((struct sockaddr_in *,
											 unsigned int, unsigned int, unsigned int,
											 xdrproc_t, caddr_t,
											 xdrproc_t, caddr_t,
											 struct timeval, unsigned int *));
extern enum clnt_stat	clnt_broadcast	__P((unsigned int, unsigned int, unsigned int,
											 xdrproc_t, char *,
											 xdrproc_t, char *,
											 bool_t (*)()));
extern unsigned short		pmap_getport	__P((struct sockaddr_in *,
												 unsigned int, unsigned int, unsigned int));
#else
extern bool_t		pmap_set	__P((unsigned long, unsigned long, int, int));
extern bool_t		pmap_unset	__P((unsigned long, unsigned long));
extern enum clnt_stat	pmap_rmtcall	__P((struct sockaddr_in *,
											 unsigned long, unsigned long, unsigned long,
											 xdrproc_t, caddr_t,
											 xdrproc_t, caddr_t,
											 struct timeval, unsigned long *));
extern enum clnt_stat	clnt_broadcast	__P((unsigned long, unsigned long, unsigned long,
											 xdrproc_t, char *,
											 xdrproc_t, char *,
											 bool_t (*)()));
extern unsigned short		pmap_getport	__P((struct sockaddr_in *,
												 unsigned long, unsigned long, unsigned int));
#endif
extern struct pmaplist	*pmap_getmaps	__P((struct sockaddr_in *));
extern int		getrpcport	__P((char *, int, int, int));
__END_DECLS

#endif /* !_RPC_PMAPCLNT_H */

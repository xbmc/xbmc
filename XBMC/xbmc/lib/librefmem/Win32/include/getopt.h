/*
 * Copyright (c) 1987, 1993, 1994, 1996
 *  The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __GETOPT_H__
#define __GETOPT_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int   opterr;      /* if error message should be printed */
extern int   optind;      /* index into parent argv vector */
extern int   optopt;      /* character checked for validity */
extern int   optreset;    /* reset getopt */
extern char *optarg;      /* argument associated with option */

int getopt (int, char * const *, const char *);

#ifdef __cplusplus
}
#endif

#endif /* __GETOPT_H__ */

#ifndef __UNISTD_GETOPT__
#ifndef __GETOPT_LONG_H__
#define __GETOPT_LONG_H__

#ifdef __cplusplus
extern "C" {
#endif

struct option {
	const char *name;
	int  has_arg;
	int *flag;
	int val;
};

int getopt_long (int, char *const *, const char *, const struct option *, int *);
#ifndef HAVE_DECL_GETOPT
#define HAVE_DECL_GETOPT 1
#endif

#define no_argument             0
#define required_argument       1
#define optional_argument       2

#ifdef __cplusplus
}
#endif

#endif /* __GETOPT_LONG_H__ */
#endif /* __UNISTD_GETOPT__ */

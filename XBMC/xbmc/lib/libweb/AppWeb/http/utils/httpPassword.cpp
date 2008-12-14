///
///	@file 	httpPassword.cpp
/// @brief 	Manage passwords for HTTP authorization.
///	@overview This file provides facilities for creating passwords for Appweb.
///		It uses basic encoding and decoding using the base64 encoding scheme 
//		and the MD5 Message-Digest algorithms developed by RSA. This module is 
///		used by both basic and digest authentication services. The 
///		base64 encode/decode algorithms originally from GoAhead.
///
/////////////////////////////////// Copyright //////////////////////////////////
//
//	@copy	default.g.r
//	
//	MD5C.C - RSA Data Security, Inc., MD5 message-digest algorithm
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	Portions Copyright (C) 1991-2, RSA Data Security, Inc. All rights reserved. 
//	Portions Copyright (C) 1995-2000, GoAhead Software. All rights reserved. 
//	
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions
//	are met:
//	
//	1. Redistributions of source code must retain the above copyright
//	   notice, this list of conditions and the following disclaimer.
//	
//	2. Redistributions in binary form must reproduce the above copyright
//	   notice, this list of conditions and the following disclaimer in the
//	   documentation and/or other materials provided with the distribution.
//	
//	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
//	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
//	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
//	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
//	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
//	SUCH DAMAGE.
//	
//	RSA License Details
//	-------------------
//	
//	License to copy and use this software is granted provided that it is 
//	identified as the "RSA Data Security, Inc. MD5 Message-Digest Algorithm" 
//	in all material mentioning or referencing this software or this function.
//	
//	License is also granted to make and use derivative works provided that such
//	works are identified as "derived from the RSA Data Security, Inc. MD5 
//	Message-Digest Algorithm" in all material mentioning or referencing the 
//	derived work.
//	
//	RSA Data Security, Inc. makes no representations concerning either the 
//	merchantability of this software or the suitability of this software for 
//	any particular purpose. It is provided "as is" without express or implied 
//	warranty of any kind.
//	
//	These notices must be retained in any copies of any part of this
//	documentation and/or software.
//	
//	@end
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
////////////////////////////////// Includes ////////////////////////////////////

#include	<ctype.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/stat.h>
#include	<time.h>

#if WIN
#include	<conio.h>
#include	<io.h>
#include	<process.h>
#define 	R_OK		4
#define 	W_OK		2
#define		MPR_TEXT	"t"
#else
#include	<unistd.h>
#define		MPR_TEXT	""
#if !CYGWIN
#define		O_BINARY	0
#define		O_TEXT		0
#endif
#endif
#if VXWORKS
#include	<ioLib.h>
#endif

#include	"posixRemap.h"

////////////////////////////// Forward Declarations ////////////////////////////

static void	addUser(char *user, char *realm, char *password, bool enabled);
static char	*getPassword(char *passBuf, int passLen);
static char *mprGetBaseName(char *name);
static char *mprMakeTempFileName(char *buf, int bufsize, char *tempDir);
static char *mprStrTok(char *str, const char *delim, char **tok);
static void printUsage(char *programName);
static int	readPassFile(char *passFile);
static char* trimWhiteSpace(char *str);
static int	updatePassFile(char *passFile);

typedef struct {
	unsigned int state[4];
	unsigned int count[2];
	unsigned char buffer[64];
} MD5_CONTEXT;

static char	*maMD5(char *string);
static void maMD5Init(MD5_CONTEXT *);
static void maMD5Update(MD5_CONTEXT *, unsigned char *, unsigned int);
static void maMD5Final(unsigned char [16], MD5_CONTEXT *);
static char	*maMD5binary(unsigned char *buf, int length);

#if WIN || VXWORKS
static char *getpass(char *prompt);
#endif

///////////////////////////////// User Class ///////////////////////////////////

class User {
  public:
	char	*name;
	char	*realm;
	char	*password;
	bool	enabled;
	User	*next;
  
			User(char *user, char *realm, char *pass, bool enabled) {
				name = strdup(user);
				this->realm = strdup(realm);
				password = strdup(pass);
				this->enabled = enabled;
			};
			~User() {};
	char	*getName() { return name; };
	char	*getRealm() { return realm; };
	char	*getPassword() { return password; };
	bool	getEnabled() { return enabled; };
	void	setPassword(char *pass) { 
				if (password) {
					free(password);
				}
				password = strdup(pass); 
			};
	void	setEnabled(bool enable) { 
				enabled = enable;
			};
};

#define MAX_PASS	64

/////////////////////////////////// Locals /////////////////////////////////////

static User 	*users;
static char		*programName;

//////////////////////////////////// Code //////////////////////////////////////

int main(int argc, char *argv[])
{
	char	passBuf[MAX_PASS], buf[MAX_PASS * 2];
	char	*password, *passFile, *userName;
	char	*encodedPassword, *realm, *cp;
	int		i, errflg, create, nextArg;
	bool	enable;

	programName = mprGetBaseName(argv[0]);
	userName = 0;
	create = errflg = 0;
	password = 0;
	enable = 1;

	for (i = 1; i < argc && !errflg; i++) {
		if (argv[i][0] != '-') {
			break;
		}

		for (cp = &argv[i][1]; *cp && !errflg; cp++) {

			if (*cp == 'c') {
				create++;

			} else if (*cp == 'e') {
				enable = 1;

			} else if (*cp == 'd') {
				enable = 0;

			} else if (*cp == 'p') {
				if (++i == argc) {
					errflg++;
				} else {
					password = argv[i];
					break;
				}

			} else {
				errflg++;
			}
		}
	}
	nextArg = i;

	if ((nextArg + 3) > argc) {
		errflg++;
	}

	if (errflg) {
		printUsage(programName);
		exit(2);
	}	

	passFile = argv[nextArg++];
	realm = argv[nextArg++];
	userName = argv[nextArg++];

	if (!create) {
		if (readPassFile(passFile) < 0) {
			exit(2);
		}
		if (access(passFile, R_OK) != 0) {
			fprintf(stderr, "%s: Can't find %s", programName, passFile);
			exit(3);
		}
		if (access(passFile, W_OK) < 0) {
			fprintf(stderr, "%s: Can't write to %s", programName, passFile);
			exit(4);
		}
	} else {
		if (access(passFile, R_OK) == 0) {
			fprintf(stderr, "%s: Can't create %s, already exists", 
				programName, passFile);
			exit(5);
		}
	}

	if (password == 0) {
		password = getPassword(passBuf, sizeof(passBuf));
		if (password == 0) {
			exit(1);
		}
	}

	sprintf(buf, "%s:%s:%s", userName, realm, password);
	encodedPassword = maMD5(buf);

	addUser(userName, realm, encodedPassword, enable);

	if (updatePassFile(passFile) < 0) {
		exit(6);
	}

	if (encodedPassword) {
		free(encodedPassword);
	}
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

static int readPassFile(char *passFile)
{
	FILE	*fp;
	char	buf[MAX_PASS * 2];
	char	*tok, *enabledSpec, *user, *realm, *password;
	bool	enabled;
	int		line;

	fp = fopen(passFile, "r" MPR_TEXT);
	if (fp == 0) {
		fprintf(stderr, "%s: Can't open %s", programName, passFile);
		return -1;
	}
	line = 0;
	while (fgets(buf, sizeof(buf), fp) != 0) {
		line++;
		enabledSpec = mprStrTok(buf, ":", &tok);
		user = mprStrTok(0, ":", &tok);
		realm = mprStrTok(0, ":", &tok);
		password = mprStrTok(0, "\n\r", &tok);
		if (enabledSpec == 0 || user == 0 || realm == 0 || password == 0) {
			fprintf(stderr,
				"%s: Badly formed password on line %d", programName, line);
			return -1;
		}
		user = trimWhiteSpace(user);
		if (*user == '#' || *user == '\0') {
			continue;
		}
		enabled = (enabledSpec[0] == '1'); 
		
		realm = trimWhiteSpace(realm);
		password = trimWhiteSpace(password);

		User	*newUser;
		newUser = new User(user, realm, password, enabled);
		newUser->next = users;
		users = newUser;
	}
	fclose(fp);
	return 0;
}
 
////////////////////////////////////////////////////////////////////////////////

static void addUser(char *user, char *realm, char *password, bool enabled)
{
	User	*up;

	up = users;
	while (up) {
		if (strcmp(user, up->getName()) == 0 && 
				strcmp(realm, up->getRealm()) == 0) {
			up->setPassword(password);
			up->setEnabled(enabled);
			return;
		}
		up = up->next;
	}

	User	*newUser;
	newUser = new User(user, realm, password, enabled);
	newUser->next = users;
	users = newUser;
}

////////////////////////////////////////////////////////////////////////////////

static int updatePassFile(char *passFile)
{
	User	*up;
	char	tempFile[256], buf[512];
	int		fd;

	mprMakeTempFileName(tempFile, sizeof(tempFile), 0);
	fd = open(tempFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);
	if (fd < 0) {
		fprintf(stderr, "%s: Can't open %s", programName, tempFile);
		return -1;
	}
	up = users;
	while (up) {
		sprintf(buf, "%d: %s: %s: %s\n", up->getEnabled(), up->getName(), 
				up->getRealm(), up->getPassword());
		if (write(fd, buf, strlen(buf)) < 0) {
			fprintf(stderr, "%s: Can't write to %s", programName, tempFile);
			return -1;
		}
		up = up->next;
	}
	close(fd);
	unlink(passFile);
	if (rename(tempFile, passFile) < 0) {
		fprintf(stderr, "%s: Can't rename %s to %s", programName, tempFile, 
			passFile);
		return -1;
	}
	return 0;
}
 
////////////////////////////////////////////////////////////////////////////////

static char *getPassword(char *passBuf, int passLen)
{
	char	*password, *confirm;

	password = getpass("New password: ");
	strncpy(passBuf, password, passLen - 1);
	confirm = getpass("Confirm password: ");
	if (strcmp(passBuf, confirm) == 0) {
		return passBuf;
	}
	fprintf(stderr, "%s: Error: Password not verified\n", programName);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
#if WIN || VXWORKS

static char *getpass(char *prompt)
{
    static char password[MAX_PASS];
    int		c, i;

    fputs(prompt, stderr);
	for (i = 0; i < (int) sizeof(password) - 1; i++) {
#if VXWORKS
		c = getchar();
#else
		c = _getch();
#endif
		if (c == '\r' || c == EOF) {
			break;
		}
		if ((c == '\b' || c == 127) && i > 0) {
			password[--i] = '\0';
			fputs("\b \b", stderr);
			i--;
		} else if (c == 26) {			// Control Z
			c = EOF;
			break;
		} else if (c == 3) {			// Control C
			fputs("^C\n", stderr);
			exit(255);
		} else if (!iscntrl(c) && (i < (int) sizeof(password) - 1)) {
			password[i] = c;
			fputc('*', stderr);
		} else {
			fputc('', stderr);
			i--;
		}
    }
	if (c == EOF) {
		return "";
	}
    fputc('\n', stderr);
    password[i] = '\0';
    return password;
}

#endif //WIN
////////////////////////////////////////////////////////////////////////////////
//
//	Display the usage
//

static void printUsage(char *programName)
{
	fprintf(stderr, 
		"usage: %s [-c] [-p password] passwordFile realm user\n", programName);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "    -c              Create the password file\n");
	fprintf(stderr, "    -p passWord     Use the specified password\n");
	fprintf(stderr, "    -e              Enable (default)\n");
	fprintf(stderr, "    -d              Disable\n");
}

////////////////////////////////////////////////////////////////////////////////

static char* trimWhiteSpace(char *str)
{
	int		len;

	if (str == 0) {
		return str;
	}
	while (isspace(*str)) {
		str++;
	}
	len = strlen(str) - 1;
	while (isspace(str[len])) {
		str[len--] = '\0';
	}
	return str;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Return the last portion of a pathname
//

static char *mprGetBaseName(char *name)
{
	char *cp;

	cp = strrchr(name, '/');

	if (cp == 0) {
		cp = strrchr(name, '\\');
		if (cp == 0) {
			return name;
		}
	} 
	if (cp == name) {
		if (cp[1] == '\0') {
			return name;
		}
	} else {
		if (cp[1] == '\0') {
			return "";
		}
	}
	return &cp[1];
}

////////////////////////////////////////////////////////////////////////////////
//
//	Thread-safe wrapping of strtok. Note "str" is modifed as per strtok()
//

static char *mprStrTok(char *str, const char *delim, char **tok)
{
	char	*start, *end;
	int		i;

	start = str ? str : *tok;

	if (start == 0) {
		return 0;
	}
	
	i = strspn(start, delim);
	start += i;
	if (*start == '\0') {
		*tok = 0;
		return 0;
	}
	end = strpbrk(start, delim);
	if (end) {
		*end++ = '\0';
		i = strspn(end, delim);
		end += i;
	}
	*tok = end;
	return start;
}

////////////////////////////////////////////////////////////////////////////////

static char *mprMakeTempFileName(char *buf, int bufsize, char *tempDir)
{
	static int 	seed = 0;
	int 		fd, i;

	for (i = 0; i < 128; i++) {
		sprintf(buf, ".pass_%d_%d.tmp", getpid(), seed++);
		fd = open(buf, O_CREAT | O_EXCL | O_BINARY, 0664);
		if (fd >= 0) {
			break;
		}
	}
	if (fd < 0) {
		fprintf(stderr, "%s: Can't make temp file %s\n", programName, buf);
		return 0;
	}

	close(fd);

	return buf;
}

////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// MD5 Code /////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
//	Constants for MD5Transform routine.
//
#define CRYPT_HASH_SIZE 16

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

static unsigned char PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// F, G, H and I are basic MD5 functions.
 
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

// ROTATE_LEFT rotates x left n bits.
 
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

// FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
// Rotation is separate from addition to prevent recomputation.
 
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (unsigned int)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (unsigned int)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (unsigned int)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (unsigned int)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

//////////////////////////////// Base 64 Data //////////////////////////////////
#if 0

#define CRYPT_HASH_SIZE   16

//
//	Encoding map lookup
//
static char	encodeMap[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
	'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
	'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/',
};

//
//	Decode map
//
static signed char decodeMap[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

#endif
//////////////////////////// Forward Declarations //////////////////////////////

static void MD5Transform(unsigned int [4], unsigned char [64]);
static void Encode(unsigned char *, unsigned int *, unsigned);
static void Decode(unsigned int *, unsigned char *, unsigned);
static void MD5_memcpy(unsigned char *, unsigned char *, unsigned);
static void MD5_memset(unsigned char *, int, unsigned);

//////////////////////////////////// Code //////////////////////////////////////
//
//	maMD5binary returns the MD5 hash. FUTURE -- better name
//

char *maMD5binary(unsigned char *buf, int length)
{
    const char		*hex = "0123456789abcdef";
    MD5_CONTEXT		md5ctx;
    unsigned char	hash[CRYPT_HASH_SIZE];
    char			*r, *str;
	char			result[(CRYPT_HASH_SIZE * 2) + 1];
    int				i;

	//
	//	Take the MD5 hash of the string argument.
	//
    maMD5Init(&md5ctx);
    maMD5Update(&md5ctx, buf, (unsigned) length);
    maMD5Final(hash, &md5ctx);

    for (i = 0, r = result; i < 16; i++) {
		*r++ = hex[hash[i] >> 4];
		*r++ = hex[hash[i] & 0xF];
    }
    *r = '\0';

	str = (char*) malloc(sizeof(result));
	strcpy(str, result);

    return str;
}

///////////////////////////////////////////////////////////////////////////////
//
//	Convenience call to webMD5binary 
// 

char *maMD5(char *string)
{
	return maMD5binary((unsigned char*)string, strlen(string));
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 initialization. Begins an MD5 operation, writing a new context.
// 

void maMD5Init(MD5_CONTEXT *context)
{
	context->count[0] = context->count[1] = 0;

	//
	// Load constants
	//
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 block update operation. Continues an MD5 message-digest operation, 
//	processing another message block, and updating the context.
//

void maMD5Update(MD5_CONTEXT *context, unsigned char *input, unsigned inputLen)
{
	unsigned 	i, index, partLen;

	index = (unsigned) ((context->count[0] >> 3) & 0x3F);

	if ((context->count[0] += ((unsigned int) inputLen << 3)) < 
			((unsigned int) inputLen << 3)){
		context->count[1]++;
	}
	context->count[1] += ((unsigned int)inputLen >> 29);
	partLen = 64 - index;

	if (inputLen >= partLen) {
		MD5_memcpy((unsigned char*) &context->buffer[index], 
			(unsigned char*) input, partLen);
		MD5Transform(context->state, context->buffer);

		for (i = partLen; i + 63 < inputLen; i += 64) {
			MD5Transform (context->state, &input[i]);
		}
		index = 0;
	} else {
		i = 0;
	}

	MD5_memcpy((unsigned char*) &context->buffer[index], 
		(unsigned char*) &input[i], inputLen-i);
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 finalization. Ends an MD5 message-digest operation, writing the message
//	digest and zeroizing the context.
// 

void maMD5Final(unsigned char digest[16], MD5_CONTEXT *context)
{
	unsigned char 	bits[8];
	unsigned		index, padLen;

	// Save number of bits 
	Encode(bits, context->count, 8);

	// Pad out to 56 mod 64.
	index = (unsigned)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	maMD5Update(context, PADDING, padLen);

	// Append length (before padding) 
	maMD5Update(context, bits, 8);
	// Store state in digest 
	Encode(digest, context->state, 16);

	// Zeroize sensitive information.
	MD5_memset((unsigned char*)context, 0, sizeof (*context));
}

////////////////////////////////////////////////////////////////////////////////
//
//	MD5 basic transformation. Transforms state based on block.
//
 
static void MD5Transform(unsigned int state[4], unsigned char block[64])
{
	unsigned int a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	Decode (x, block, 64);

	// Round 1 
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); // 1 
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); // 2 
	FF (c, d, a, b, x[ 2], S13, 0x242070db); // 3 
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); // 4 
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); // 5 
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); // 6 
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); // 7 
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); // 8 
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); // 9 
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); // 10 
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); // 11 
	FF (b, c, d, a, x[11], S14, 0x895cd7be); // 12 
	FF (a, b, c, d, x[12], S11, 0x6b901122); // 13 
	FF (d, a, b, c, x[13], S12, 0xfd987193); // 14 
	FF (c, d, a, b, x[14], S13, 0xa679438e); // 15 
	FF (b, c, d, a, x[15], S14, 0x49b40821); // 16 

	// Round 2 
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); // 17 
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); // 18 
	GG (c, d, a, b, x[11], S23, 0x265e5a51); // 19 
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); // 20 
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); // 21 
	GG (d, a, b, c, x[10], S22,  0x2441453); // 22 
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); // 23 
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); // 24 
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); // 25 
	GG (d, a, b, c, x[14], S22, 0xc33707d6); // 26 
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); // 27 
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); // 28 
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); // 29 
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); // 30 
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); // 31 
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); // 32 

	// Round 3 
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); // 33 
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); // 34 
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); // 35 
	HH (b, c, d, a, x[14], S34, 0xfde5380c); // 36 
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); // 37 
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); // 38 
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); // 39 
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); // 40 
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); // 41 
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); // 42 
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); // 43 
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); // 44 
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); // 45 
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); // 46 
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); // 47 
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); // 48 

	// Round 4 
	II (a, b, c, d, x[ 0], S41, 0xf4292244); // 49 
	II (d, a, b, c, x[ 7], S42, 0x432aff97); // 50 
	II (c, d, a, b, x[14], S43, 0xab9423a7); // 51 
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); // 52 
	II (a, b, c, d, x[12], S41, 0x655b59c3); // 53 
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); // 54 
	II (c, d, a, b, x[10], S43, 0xffeff47d); // 55 
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); // 56 
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); // 57 
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); // 58 
	II (c, d, a, b, x[ 6], S43, 0xa3014314); // 59 
	II (b, c, d, a, x[13], S44, 0x4e0811a1); // 60 
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); // 61 
	II (d, a, b, c, x[11], S42, 0xbd3af235); // 62 
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); // 63 
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); // 64 

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	MD5_memset ((unsigned char*) x, 0, sizeof (x));
}

////////////////////////////////////////////////////////////////////////////////
//
//	Encodes input (unsigned int) into output (unsigned char). Assumes len is a 
//	multiple of 4.
//
 
static void Encode(unsigned char *output, unsigned int *input, unsigned len)
{
	unsigned 	i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = (unsigned char) (input[i] & 0xff);
		output[j+1] = (unsigned char) ((input[i] >> 8) & 0xff);
		output[j+2] = (unsigned char) ((input[i] >> 16) & 0xff);
		output[j+3] = (unsigned char) ((input[i] >> 24) & 0xff);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Decodes input (unsigned char) into output (unsigned int). Assumes len is a 
//	multiple of 4.
//
 
static void Decode(unsigned int *output, unsigned char *input, unsigned len)
{
	unsigned 	i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((unsigned int) input[j]) | 
			(((unsigned int) input[j+1]) << 8) |
			(((unsigned int) input[j+2]) << 16) | 
			(((unsigned int) input[j+3]) << 24);
}

////////////////////////////////////////////////////////////////////////////////
//
//	FUTURE: Replace "for loop" with standard memcpy if possible.
//

static void MD5_memcpy(unsigned char *output, unsigned char *input, 
	unsigned len)
{
	unsigned 	i;

	for (i = 0; i < len; i++)
		output[i] = input[i];
}

////////////////////////////////////////////////////////////////////////////////
//
// FUTURE: Replace "for loop" with standard memset if possible.
//
 
static void MD5_memset(unsigned char *output, int value, unsigned len)
{
	unsigned 	i;

	for (i = 0; i < len; i++)
		((char*) output)[i] = (char) value;
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//

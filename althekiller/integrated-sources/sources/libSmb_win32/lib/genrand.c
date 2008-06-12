/* 
   Unix SMB/CIFS implementation.

   Functions to create reasonable random numbers for crypto use.

   Copyright (C) Jeremy Allison 2001
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

static unsigned char smb_arc4_state[258];
static uint32 counter;

static BOOL done_reseed = False;
static void (*reseed_callback)(int *newseed);

/**************************************************************** 
 Copy any user given reseed data.
*****************************************************************/

void set_rand_reseed_callback(void (*fn)(int *))
{
	reseed_callback = fn;
	set_need_random_reseed();
}

void set_need_random_reseed(void)
{
	done_reseed = False;
}

static void get_rand_reseed_data(int *reseed_data)
{
	if (reseed_callback) {
		reseed_callback(reseed_data);
	} else {
		*reseed_data = 0;
	}
}

/****************************************************************
 Get a 16 byte hash from the contents of a file.
 Note that the hash is not initialised.
*****************************************************************/

static void do_filehash(const char *fname, unsigned char *the_hash)
{
	unsigned char buf[1011]; /* deliberate weird size */
	unsigned char tmp_md4[16];
	int fd, n;

	fd = sys_open(fname,O_RDONLY,0);
	if (fd == -1)
		return;

	while ((n = read(fd, (char *)buf, sizeof(buf))) > 0) {
		mdfour(tmp_md4, buf, n);
		for (n=0;n<16;n++)
			the_hash[n] ^= tmp_md4[n];
	}
	close(fd);
}

/**************************************************************
 Try and get a good random number seed. Try a number of
 different factors. Firstly, try /dev/urandom - use if exists.

 We use /dev/urandom as a read of /dev/random can block if
 the entropy pool dries up. This leads clients to timeout
 or be very slow on connect.

 If we can't use /dev/urandom then seed the stream random generator
 above...
**************************************************************/

static int do_reseed(BOOL use_fd, int fd)
{
	unsigned char seed_inbuf[40];
	uint32 v1, v2; struct timeval tval; pid_t mypid;
	struct passwd *pw;
	int reseed_data = 0;

	if (use_fd) {
		if (fd != -1)
			return fd;

		fd = sys_open( "/dev/urandom", O_RDONLY,0);
		if(fd >= 0)
			return fd;
	}

	/* Add in some secret file contents */

	do_filehash("/etc/shadow", &seed_inbuf[0]);
	do_filehash(lp_smb_passwd_file(), &seed_inbuf[16]);

	/*
	 * Add in the root encrypted password.
	 * On any system where security is taken
	 * seriously this will be secret.
	 */

	pw = getpwnam_alloc(NULL, "root");
	if (pw && pw->pw_passwd) {
		size_t i;
		unsigned char md4_tmp[16];
		mdfour(md4_tmp, (unsigned char *)pw->pw_passwd, strlen(pw->pw_passwd));
		for (i=0;i<16;i++)
			seed_inbuf[8+i] ^= md4_tmp[i];
		TALLOC_FREE(pw);
	}

	/*
	 * Add the counter, time of day, and pid.
	 */

	GetTimeOfDay(&tval);
	mypid = sys_getpid();
	v1 = (counter++) + mypid + tval.tv_sec;
	v2 = (counter++) * mypid + tval.tv_usec;

	SIVAL(seed_inbuf, 32, v1 ^ IVAL(seed_inbuf, 32));
	SIVAL(seed_inbuf, 36, v2 ^ IVAL(seed_inbuf, 36));

	/*
	 * Add any user-given reseed data.
	 */

	get_rand_reseed_data(&reseed_data);
	if (reseed_data) {
		size_t i;
		for (i = 0; i < sizeof(seed_inbuf); i++)
			seed_inbuf[i] ^= ((char *)(&reseed_data))[i % sizeof(reseed_data)];
	}

	smb_arc4_init(smb_arc4_state, seed_inbuf, sizeof(seed_inbuf));

	return -1;
}

/*******************************************************************
 Interface to the (hopefully) good crypto random number generator.
********************************************************************/

void generate_random_buffer( unsigned char *out, int len)
{
	static int urand_fd = -1;
	unsigned char md4_buf[64];
	unsigned char tmp_buf[16];
	unsigned char *p;

	if(!done_reseed) {
		urand_fd = do_reseed(True, urand_fd);
		done_reseed = True;
	}

	if (urand_fd != -1 && len > 0) {

		if (read(urand_fd, out, len) == len)
			return; /* len bytes of random data read from urandom. */

		/* Read of urand error, drop back to non urand method. */
		close(urand_fd);
		urand_fd = -1;
		do_reseed(False, -1);
		done_reseed = True;
	}

	/*
	 * Generate random numbers in chunks of 64 bytes,
	 * then md4 them & copy to the output buffer.
	 * This way the raw state of the stream is never externally
	 * seen.
	 */

	p = out;
	while(len > 0) {
		int copy_len = len > 16 ? 16 : len;

		smb_arc4_crypt(smb_arc4_state, md4_buf, sizeof(md4_buf));
		mdfour(tmp_buf, md4_buf, sizeof(md4_buf));
		memcpy(p, tmp_buf, copy_len);
		p += copy_len;
		len -= copy_len;
	}
}

/*******************************************************************
 Use the random number generator to generate a random string.
********************************************************************/

static char c_list[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+_-#.,";

char *generate_random_str(size_t len)
{
	static unsigned char retstr[256];
	size_t i;

	memset(retstr, '\0', sizeof(retstr));

	if (len > sizeof(retstr)-1)
		len = sizeof(retstr) -1;
	generate_random_buffer( retstr, len);
	for (i = 0; i < len; i++)
		retstr[i] = c_list[ retstr[i] % (sizeof(c_list)-1) ];

	retstr[i] = '\0';

	return (char *)retstr;
}

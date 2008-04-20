/* 
   Unix SMB/CIFS implementation.
   code to manipulate domain credentials
   Copyright (C) Andrew Tridgell 1997-1998
   Largely rewritten by Jeremy Allison 2005.
   
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

/****************************************************************************
 Represent a credential as a string.
****************************************************************************/

char *credstr(const unsigned char *cred)
{
	static fstring buf;
	slprintf(buf, sizeof(buf) - 1, "%02X%02X%02X%02X%02X%02X%02X%02X",
		cred[0], cred[1], cred[2], cred[3], 
		cred[4], cred[5], cred[6], cred[7]);
	return buf;
}

/****************************************************************************
 Setup the session key and the client and server creds in dc.
 ADS-style 128 bit session keys.
 Used by both client and server creds setup.
****************************************************************************/

static void creds_init_128(struct dcinfo *dc,
				const DOM_CHAL *clnt_chal_in,
				const DOM_CHAL *srv_chal_in,
				const unsigned char mach_pw[16])
{
	unsigned char zero[4], tmp[16];
	HMACMD5Context ctx;
	struct MD5Context md5;

	/* Just in case this isn't already there */
	memcpy(dc->mach_pw, mach_pw, 16);

	ZERO_STRUCT(dc->sess_key);

	memset(zero, 0, sizeof(zero));

	hmac_md5_init_rfc2104(mach_pw, 16, &ctx);
	MD5Init(&md5);
	MD5Update(&md5, zero, sizeof(zero));
	MD5Update(&md5, clnt_chal_in->data, 8);
	MD5Update(&md5, srv_chal_in->data, 8);
	MD5Final(tmp, &md5);
	hmac_md5_update(tmp, sizeof(tmp), &ctx);
	hmac_md5_final(dc->sess_key, &ctx);

	/* debug output */
	DEBUG(5,("creds_init_128\n"));
	DEBUG(5,("\tclnt_chal_in: %s\n", credstr(clnt_chal_in->data)));
	DEBUG(5,("\tsrv_chal_in : %s\n", credstr(srv_chal_in->data)));
	dump_data_pw("\tsession_key ", (const unsigned char *)dc->sess_key, 16);

	/* Generate the next client and server creds. */
	
	des_crypt112(dc->clnt_chal.data,		/* output */
			clnt_chal_in->data,		/* input */
			dc->sess_key,			/* input */
			1);

	des_crypt112(dc->srv_chal.data,			/* output */
			srv_chal_in->data,		/* input */
			dc->sess_key,			/* input */
			1);

	/* Seed is the client chal. */
	memcpy(dc->seed_chal.data, dc->clnt_chal.data, 8);
}

/****************************************************************************
 Setup the session key and the client and server creds in dc.
 Used by both client and server creds setup.
****************************************************************************/

static void creds_init_64(struct dcinfo *dc,
			const DOM_CHAL *clnt_chal_in,
			const DOM_CHAL *srv_chal_in,
			const unsigned char mach_pw[16])
{
	uint32 sum[2];
	unsigned char sum2[8];

	/* Just in case this isn't already there */
	memcpy(dc->mach_pw, mach_pw, 16);

	sum[0] = IVAL(clnt_chal_in->data, 0) + IVAL(srv_chal_in->data, 0);
	sum[1] = IVAL(clnt_chal_in->data, 4) + IVAL(srv_chal_in->data, 4);

	SIVAL(sum2,0,sum[0]);
	SIVAL(sum2,4,sum[1]);

	ZERO_STRUCT(dc->sess_key);

	des_crypt128(dc->sess_key, sum2, dc->mach_pw);

	/* debug output */
	DEBUG(5,("creds_init_64\n"));
	DEBUG(5,("\tclnt_chal_in: %s\n", credstr(clnt_chal_in->data)));
	DEBUG(5,("\tsrv_chal_in : %s\n", credstr(srv_chal_in->data)));
	DEBUG(5,("\tclnt+srv : %s\n", credstr(sum2)));
	DEBUG(5,("\tsess_key_out : %s\n", credstr(dc->sess_key)));

	/* Generate the next client and server creds. */
	
	des_crypt112(dc->clnt_chal.data,		/* output */
			clnt_chal_in->data,		/* input */
			dc->sess_key,			/* input */
			1);

	des_crypt112(dc->srv_chal.data,			/* output */
			srv_chal_in->data,		/* input */
			dc->sess_key,			/* input */
			1);

	/* Seed is the client chal. */
	memcpy(dc->seed_chal.data, dc->clnt_chal.data, 8);
}

/****************************************************************************
 Utility function to step credential chain one forward.
 Deliberately doesn't update the seed. See reseed comment below.
****************************************************************************/

static void creds_step(struct dcinfo *dc)
{
	DOM_CHAL time_chal;

	DEBUG(5,("\tsequence = 0x%x\n", (unsigned int)dc->sequence ));

	DEBUG(5,("\tseed:        %s\n", credstr(dc->seed_chal.data) ));

	SIVAL(time_chal.data, 0, IVAL(dc->seed_chal.data, 0) + dc->sequence);
	SIVAL(time_chal.data, 4, IVAL(dc->seed_chal.data, 4));
                                                                                                   
	DEBUG(5,("\tseed+seq   %s\n", credstr(time_chal.data) ));

	des_crypt112(dc->clnt_chal.data, time_chal.data, dc->sess_key, 1);

	DEBUG(5,("\tCLIENT      %s\n", credstr(dc->clnt_chal.data) ));

	SIVAL(time_chal.data, 0, IVAL(dc->seed_chal.data, 0) + dc->sequence + 1);
	SIVAL(time_chal.data, 4, IVAL(dc->seed_chal.data, 4));

	DEBUG(5,("\tseed+seq+1   %s\n", credstr(time_chal.data) ));

	des_crypt112(dc->srv_chal.data, time_chal.data, dc->sess_key, 1);

	DEBUG(5,("\tSERVER      %s\n", credstr(dc->srv_chal.data) ));
}

/****************************************************************************
 Create a server credential struct.
****************************************************************************/

void creds_server_init(uint32 neg_flags,
			struct dcinfo *dc,
			DOM_CHAL *clnt_chal,
			DOM_CHAL *srv_chal,
			const unsigned char mach_pw[16],
			DOM_CHAL *init_chal_out)
{
	DEBUG(10,("creds_server_init: neg_flags : %x\n", (unsigned int)neg_flags));
	DEBUG(10,("creds_server_init: client chal : %s\n", credstr(clnt_chal->data) ));
	DEBUG(10,("creds_server_init: server chal : %s\n", credstr(srv_chal->data) ));
	dump_data_pw("creds_server_init: machine pass", mach_pw, 16);

	/* Generate the session key and the next client and server creds. */
	if (neg_flags & NETLOGON_NEG_128BIT) {
		creds_init_128(dc,
			clnt_chal,
			srv_chal,
			mach_pw);
	} else {
		creds_init_64(dc,
			clnt_chal,
			srv_chal,
			mach_pw);
	}

	dump_data_pw("creds_server_init: session key", dc->sess_key, 16);

	DEBUG(10,("creds_server_init: clnt : %s\n", credstr(dc->clnt_chal.data) ));
	DEBUG(10,("creds_server_init: server : %s\n", credstr(dc->srv_chal.data) ));
	DEBUG(10,("creds_server_init: seed : %s\n", credstr(dc->seed_chal.data) ));

	memcpy(init_chal_out->data, dc->srv_chal.data, 8);
}

/****************************************************************************
 Check a credential sent by the client.
****************************************************************************/

BOOL creds_server_check(const struct dcinfo *dc, const DOM_CHAL *rcv_cli_chal_in)
{
	if (memcmp(dc->clnt_chal.data, rcv_cli_chal_in->data, 8)) {
		DEBUG(5,("creds_server_check: challenge : %s\n", credstr(rcv_cli_chal_in->data)));
		DEBUG(5,("calculated: %s\n", credstr(dc->clnt_chal.data)));
		DEBUG(2,("creds_server_check: credentials check failed.\n"));
		return False;
	}
	DEBUG(10,("creds_server_check: credentials check OK.\n"));
	return True;
}

/****************************************************************************
 Replace current seed chal. Internal function - due to split server step below.
****************************************************************************/

static void creds_reseed(struct dcinfo *dc)
{
	DOM_CHAL time_chal;

	SIVAL(time_chal.data, 0, IVAL(dc->seed_chal.data, 0) + dc->sequence + 1);
	SIVAL(time_chal.data, 4, IVAL(dc->seed_chal.data, 4));

	dc->seed_chal = time_chal;

	DEBUG(5,("cred_reseed: seed %s\n", credstr(dc->seed_chal.data) ));
}

/****************************************************************************
 Step the server credential chain one forward. 
****************************************************************************/

BOOL creds_server_step(struct dcinfo *dc, const DOM_CRED *received_cred, DOM_CRED *cred_out)
{
	BOOL ret;
	struct dcinfo tmp_dc = *dc;

	/* Do all operations on a temporary copy of the dc,
	   which we throw away if the checks fail. */

	tmp_dc.sequence = received_cred->timestamp.time;

	creds_step(&tmp_dc);

	/* Create the outgoing credentials */
	cred_out->timestamp.time = tmp_dc.sequence + 1;
	cred_out->challenge = tmp_dc.srv_chal;

	creds_reseed(&tmp_dc);

	ret = creds_server_check(&tmp_dc, &received_cred->challenge);
	if (!ret) {
		return False;
	}

	/* creds step succeeded - replace the current creds. */
	*dc = tmp_dc;
	return True;
}

/****************************************************************************
 Create a client credential struct.
****************************************************************************/

void creds_client_init(uint32 neg_flags,
			struct dcinfo *dc,
			DOM_CHAL *clnt_chal,
			DOM_CHAL *srv_chal,
			const unsigned char mach_pw[16],
			DOM_CHAL *init_chal_out)
{
	dc->sequence = time(NULL);

	DEBUG(10,("creds_client_init: neg_flags : %x\n", (unsigned int)neg_flags));
	DEBUG(10,("creds_client_init: client chal : %s\n", credstr(clnt_chal->data) ));
	DEBUG(10,("creds_client_init: server chal : %s\n", credstr(srv_chal->data) ));
	dump_data_pw("creds_client_init: machine pass", (const unsigned char *)mach_pw, 16);

	/* Generate the session key and the next client and server creds. */
	if (neg_flags & NETLOGON_NEG_128BIT) {
		creds_init_128(dc,
				clnt_chal,
				srv_chal,
				mach_pw);
	} else {
		creds_init_64(dc,
			clnt_chal,
			srv_chal,
			mach_pw);
	}

	dump_data_pw("creds_client_init: session key", dc->sess_key, 16);

	DEBUG(10,("creds_client_init: clnt : %s\n", credstr(dc->clnt_chal.data) ));
	DEBUG(10,("creds_client_init: server : %s\n", credstr(dc->srv_chal.data) ));
	DEBUG(10,("creds_client_init: seed : %s\n", credstr(dc->seed_chal.data) ));

	memcpy(init_chal_out->data, dc->clnt_chal.data, 8);
}

/****************************************************************************
 Check a credential returned by the server.
****************************************************************************/

BOOL creds_client_check(const struct dcinfo *dc, const DOM_CHAL *rcv_srv_chal_in)
{
	if (memcmp(dc->srv_chal.data, rcv_srv_chal_in->data, 8)) {
		DEBUG(5,("creds_client_check: challenge : %s\n", credstr(rcv_srv_chal_in->data)));
		DEBUG(5,("calculated: %s\n", credstr(dc->srv_chal.data)));
		DEBUG(0,("creds_client_check: credentials check failed.\n"));
		return False;
	}
	DEBUG(10,("creds_client_check: credentials check OK.\n"));
	return True;
}

/****************************************************************************
  Step the client credentials to the next element in the chain, updating the
  current client and server credentials and the seed
  produce the next authenticator in the sequence ready to send to
  the server
****************************************************************************/

void creds_client_step(struct dcinfo *dc, DOM_CRED *next_cred_out)
{
        dc->sequence += 2;
	creds_step(dc);
	creds_reseed(dc);

	next_cred_out->challenge = dc->clnt_chal;
	next_cred_out->timestamp.time = dc->sequence;
}

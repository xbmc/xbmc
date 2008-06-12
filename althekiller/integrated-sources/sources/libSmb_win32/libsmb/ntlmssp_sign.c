/* 
 *  Unix SMB/CIFS implementation.
 *  Version 3.0
 *  NTLMSSP Signing routines
 *  Copyright (C) Andrew Bartlett 2003-2005
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "includes.h"

#define CLI_SIGN "session key to client-to-server signing key magic constant"
#define CLI_SEAL "session key to client-to-server sealing key magic constant"
#define SRV_SIGN "session key to server-to-client signing key magic constant"
#define SRV_SEAL "session key to server-to-client sealing key magic constant"

/**
 * Some notes on then NTLM2 code:
 *
 * NTLM2 is a AEAD system.  This means that the data encrypted is not
 * all the data that is signed.  In DCE-RPC case, the headers of the
 * DCE-RPC packets are also signed.  This prevents some of the
 * fun-and-games one might have by changing them.
 *
 */

static void calc_ntlmv2_key(unsigned char subkey[16],
				DATA_BLOB session_key,
				const char *constant)
{
	struct MD5Context ctx3;
	MD5Init(&ctx3);
	MD5Update(&ctx3, session_key.data, session_key.length);
	MD5Update(&ctx3, (const unsigned char *)constant, strlen(constant)+1);
	MD5Final(subkey, &ctx3);
}

enum ntlmssp_direction {
	NTLMSSP_SEND,
	NTLMSSP_RECEIVE
};

static NTSTATUS ntlmssp_make_packet_signature(NTLMSSP_STATE *ntlmssp_state,
						const uchar *data, size_t length, 
						const uchar *whole_pdu, size_t pdu_length,
						enum ntlmssp_direction direction,
						DATA_BLOB *sig,
						BOOL encrypt_sig)
{
	if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_NTLM2) {
		HMACMD5Context ctx;
		uchar seq_num[4];
		uchar digest[16];

		*sig = data_blob(NULL, NTLMSSP_SIG_SIZE);
		if (!sig->data) {
			return NT_STATUS_NO_MEMORY;
		}

		switch (direction) {
			case NTLMSSP_SEND:
	                        DEBUG(100,("ntlmssp_make_packet_signature: SEND seq = %u, len = %u, pdu_len = %u\n",
					ntlmssp_state->ntlm2_send_seq_num,
					(unsigned int)length,
					(unsigned int)pdu_length));

				SIVAL(seq_num, 0, ntlmssp_state->ntlm2_send_seq_num);
				ntlmssp_state->ntlm2_send_seq_num++;
				hmac_md5_init_limK_to_64(ntlmssp_state->send_sign_key, 16, &ctx);
				break;
			case NTLMSSP_RECEIVE:

				DEBUG(100,("ntlmssp_make_packet_signature: RECV seq = %u, len = %u, pdu_len = %u\n",
					ntlmssp_state->ntlm2_recv_seq_num,
					(unsigned int)length,
					(unsigned int)pdu_length));

				SIVAL(seq_num, 0, ntlmssp_state->ntlm2_recv_seq_num);
				ntlmssp_state->ntlm2_recv_seq_num++;
				hmac_md5_init_limK_to_64(ntlmssp_state->recv_sign_key, 16, &ctx);
				break;
                }

		dump_data_pw("pdu data ", whole_pdu, pdu_length);

		hmac_md5_update(seq_num, 4, &ctx);
		hmac_md5_update(whole_pdu, pdu_length, &ctx);
		hmac_md5_final(digest, &ctx);

		if (encrypt_sig && (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_KEY_EXCH)) {
			switch (direction) {
			case NTLMSSP_SEND:
				smb_arc4_crypt(ntlmssp_state->send_seal_arc4_state,  digest, 8);
				break;
			case NTLMSSP_RECEIVE:
				smb_arc4_crypt(ntlmssp_state->recv_seal_arc4_state,  digest, 8);
				break;
			}
		}

		SIVAL(sig->data, 0, NTLMSSP_SIGN_VERSION);
		memcpy(sig->data + 4, digest, 8);
		memcpy(sig->data + 12, seq_num, 4);

		dump_data_pw("ntlmssp v2 sig ", sig->data, sig->length);

	} else {
		uint32 crc;
		crc = crc32_calc_buffer((const char *)data, length);
		if (!msrpc_gen(sig, "dddd", NTLMSSP_SIGN_VERSION, 0, crc, ntlmssp_state->ntlmv1_seq_num)) {
			return NT_STATUS_NO_MEMORY;
		}
		
		ntlmssp_state->ntlmv1_seq_num++;

		dump_data_pw("ntlmssp hash:\n", ntlmssp_state->ntlmv1_arc4_state,
			     sizeof(ntlmssp_state->ntlmv1_arc4_state));
		smb_arc4_crypt(ntlmssp_state->ntlmv1_arc4_state, sig->data+4, sig->length-4);
	}
	return NT_STATUS_OK;
}

NTSTATUS ntlmssp_sign_packet(NTLMSSP_STATE *ntlmssp_state,
				    const uchar *data, size_t length, 
				    const uchar *whole_pdu, size_t pdu_length, 
				    DATA_BLOB *sig) 
{
	NTSTATUS nt_status;

	if (!(ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_SIGN)) {
		DEBUG(3, ("NTLMSSP Signing not negotiated - cannot sign packet!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!ntlmssp_state->session_key.length) {
		DEBUG(3, ("NO session key, cannot check sign packet\n"));
		return NT_STATUS_NO_USER_SESSION_KEY;
	}

	nt_status = ntlmssp_make_packet_signature(ntlmssp_state,
						data, length,
						whole_pdu, pdu_length,
						NTLMSSP_SEND, sig, True);

	return nt_status;
}

/**
 * Check the signature of an incoming packet 
 * @note caller *must* check that the signature is the size it expects 
 *
 */

NTSTATUS ntlmssp_check_packet(NTLMSSP_STATE *ntlmssp_state,
				const uchar *data, size_t length, 
				const uchar *whole_pdu, size_t pdu_length, 
				const DATA_BLOB *sig) 
{
	DATA_BLOB local_sig;
	NTSTATUS nt_status;

	if (!ntlmssp_state->session_key.length) {
		DEBUG(3, ("NO session key, cannot check packet signature\n"));
		return NT_STATUS_NO_USER_SESSION_KEY;
	}

	if (sig->length < 8) {
		DEBUG(0, ("NTLMSSP packet check failed due to short signature (%lu bytes)!\n", 
			  (unsigned long)sig->length));
	}

	nt_status = ntlmssp_make_packet_signature(ntlmssp_state,
						data, length,
						whole_pdu, pdu_length,
						NTLMSSP_RECEIVE, &local_sig, True);
	
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("NTLMSSP packet check failed with %s\n", nt_errstr(nt_status)));
		data_blob_free(&local_sig);
		return nt_status;
	}
	
	if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_NTLM2) {
		if (local_sig.length != sig->length ||
				memcmp(local_sig.data, sig->data, sig->length) != 0) {
			DEBUG(5, ("BAD SIG NTLM2: wanted signature of\n"));
			dump_data(5, (const char *)local_sig.data, local_sig.length);

			DEBUG(5, ("BAD SIG: got signature of\n"));
			dump_data(5, (const char *)sig->data, sig->length);

			DEBUG(0, ("NTLMSSP NTLM2 packet check failed due to invalid signature!\n"));
			data_blob_free(&local_sig);
			return NT_STATUS_ACCESS_DENIED;
		}
	} else {
		if (local_sig.length != sig->length ||
				memcmp(local_sig.data + 8, sig->data + 8, sig->length - 8) != 0) {
			DEBUG(5, ("BAD SIG NTLM1: wanted signature of\n"));
			dump_data(5, (const char *)local_sig.data, local_sig.length);

			DEBUG(5, ("BAD SIG: got signature of\n"));
			dump_data(5, (const char *)sig->data, sig->length);

			DEBUG(0, ("NTLMSSP NTLM1 packet check failed due to invalid signature!\n"));
			data_blob_free(&local_sig);
			return NT_STATUS_ACCESS_DENIED;
		}
	}
	dump_data_pw("checked ntlmssp signature\n", sig->data, sig->length);
	DEBUG(10,("ntlmssp_check_packet: NTLMSSP signature OK !\n"));

	data_blob_free(&local_sig);
	return NT_STATUS_OK;
}

/**
 * Seal data with the NTLMSSP algorithm
 *
 */

NTSTATUS ntlmssp_seal_packet(NTLMSSP_STATE *ntlmssp_state,
			     uchar *data, size_t length,
			     uchar *whole_pdu, size_t pdu_length,
			     DATA_BLOB *sig)
{	
	if (!(ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_SEAL)) {
		DEBUG(3, ("NTLMSSP Sealing not negotiated - cannot seal packet!\n"));
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!ntlmssp_state->session_key.length) {
		DEBUG(3, ("NO session key, cannot seal packet\n"));
		return NT_STATUS_NO_USER_SESSION_KEY;
	}

	DEBUG(10,("ntlmssp_seal_data: seal\n"));
	dump_data_pw("ntlmssp clear data\n", data, length);
	if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_NTLM2) {
		/* The order of these two operations matters - we must first seal the packet,
		   then seal the sequence number - this is becouse the send_seal_hash is not
		   constant, but is is rather updated with each iteration */
		NTSTATUS nt_status = ntlmssp_make_packet_signature(ntlmssp_state,
							data, length,
							whole_pdu, pdu_length,
							NTLMSSP_SEND, sig, False);
		if (!NT_STATUS_IS_OK(nt_status)) {
			return nt_status;
		}

		smb_arc4_crypt(ntlmssp_state->send_seal_arc4_state, data, length);
		if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_KEY_EXCH) {
			smb_arc4_crypt(ntlmssp_state->send_seal_arc4_state, sig->data+4, 8);
		}
	} else {
		uint32 crc;
		crc = crc32_calc_buffer((const char *)data, length);
		if (!msrpc_gen(sig, "dddd", NTLMSSP_SIGN_VERSION, 0, crc, ntlmssp_state->ntlmv1_seq_num)) {
			return NT_STATUS_NO_MEMORY;
		}

		/* The order of these two operations matters - we must first seal the packet,
		   then seal the sequence number - this is becouse the ntlmv1_arc4_state is not
		   constant, but is is rather updated with each iteration */
		
		dump_data_pw("ntlmv1 arc4 state:\n", ntlmssp_state->ntlmv1_arc4_state,
			     sizeof(ntlmssp_state->ntlmv1_arc4_state));
		smb_arc4_crypt(ntlmssp_state->ntlmv1_arc4_state, data, length);

		dump_data_pw("ntlmv1 arc4 state:\n", ntlmssp_state->ntlmv1_arc4_state,
			     sizeof(ntlmssp_state->ntlmv1_arc4_state));

		smb_arc4_crypt(ntlmssp_state->ntlmv1_arc4_state, sig->data+4, sig->length-4);

		ntlmssp_state->ntlmv1_seq_num++;
	}
	dump_data_pw("ntlmssp signature\n", sig->data, sig->length);
	dump_data_pw("ntlmssp sealed data\n", data, length);

	return NT_STATUS_OK;
}

/**
 * Unseal data with the NTLMSSP algorithm
 *
 */

NTSTATUS ntlmssp_unseal_packet(NTLMSSP_STATE *ntlmssp_state,
				uchar *data, size_t length,
				uchar *whole_pdu, size_t pdu_length,
				DATA_BLOB *sig)
{
	if (!ntlmssp_state->session_key.length) {
		DEBUG(3, ("NO session key, cannot unseal packet\n"));
		return NT_STATUS_NO_USER_SESSION_KEY;
	}

	DEBUG(10,("ntlmssp_unseal_data: seal\n"));
	dump_data_pw("ntlmssp sealed data\n", data, length);

	if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_NTLM2) {
		/* First unseal the data. */
		smb_arc4_crypt(ntlmssp_state->recv_seal_arc4_state, data, length);
		dump_data_pw("ntlmv2 clear data\n", data, length);
	} else {
		smb_arc4_crypt(ntlmssp_state->ntlmv1_arc4_state, data, length);
		dump_data_pw("ntlmv1 clear data\n", data, length);
	}
	return ntlmssp_check_packet(ntlmssp_state, data, length, whole_pdu, pdu_length, sig);
}

/**
   Initialise the state for NTLMSSP signing.
*/
NTSTATUS ntlmssp_sign_init(NTLMSSP_STATE *ntlmssp_state)
{
	unsigned char p24[24];
	TALLOC_CTX *mem_ctx;
	ZERO_STRUCT(p24);

	mem_ctx = talloc_init("weak_keys");
	if (!mem_ctx) {
		return NT_STATUS_NO_MEMORY;
	}

	DEBUG(3, ("NTLMSSP Sign/Seal - Initialising with flags:\n"));
	debug_ntlmssp_flags(ntlmssp_state->neg_flags);

	if (ntlmssp_state->session_key.length < 8) {
		TALLOC_FREE(mem_ctx);
		DEBUG(3, ("NO session key, cannot intialise signing\n"));
		return NT_STATUS_NO_USER_SESSION_KEY;
	}

	if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_NTLM2) {
		DATA_BLOB weak_session_key = ntlmssp_state->session_key;
		const char *send_sign_const;
		const char *send_seal_const;
		const char *recv_sign_const;
		const char *recv_seal_const;

		switch (ntlmssp_state->role) {
		case NTLMSSP_CLIENT:
			send_sign_const = CLI_SIGN;
			send_seal_const = CLI_SEAL;
			recv_sign_const = SRV_SIGN;
			recv_seal_const = SRV_SEAL;
			break;
		case NTLMSSP_SERVER:
			send_sign_const = SRV_SIGN;
			send_seal_const = SRV_SEAL;
			recv_sign_const = CLI_SIGN;
			recv_seal_const = CLI_SEAL;
			break;
		default:
			TALLOC_FREE(mem_ctx);
			return NT_STATUS_INTERNAL_ERROR;
		}

		/**
		  Weaken NTLMSSP keys to cope with down-level clients, servers and export restrictions.
		  We probably should have some parameters to control this, once we get NTLM2 working.
		*/

		if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_128) {
			;
		} else if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_56) {
			weak_session_key.length = 7;
		} else { /* forty bits */
			weak_session_key.length = 5;
		}

		dump_data_pw("NTLMSSP weakend master key:\n",
				weak_session_key.data,
				weak_session_key.length);

		/* SEND: sign key */
		calc_ntlmv2_key(ntlmssp_state->send_sign_key,
				ntlmssp_state->session_key, send_sign_const);
		dump_data_pw("NTLMSSP send sign key:\n",
				ntlmssp_state->send_sign_key, 16);

		/* SEND: seal ARCFOUR pad */
		calc_ntlmv2_key(ntlmssp_state->send_seal_key,
				weak_session_key, send_seal_const);
		dump_data_pw("NTLMSSP send seal key:\n",
				ntlmssp_state->send_seal_key, 16);

		smb_arc4_init(ntlmssp_state->send_seal_arc4_state,
				ntlmssp_state->send_seal_key, 16);

		dump_data_pw("NTLMSSP send seal arc4 state:\n", 
			     ntlmssp_state->send_seal_arc4_state, 
			     sizeof(ntlmssp_state->send_seal_arc4_state));

		/* RECV: sign key */
		calc_ntlmv2_key(ntlmssp_state->recv_sign_key,
				ntlmssp_state->session_key, recv_sign_const);
		dump_data_pw("NTLMSSP recv send sign key:\n",
				ntlmssp_state->recv_sign_key, 16);

		/* RECV: seal ARCFOUR pad */
		calc_ntlmv2_key(ntlmssp_state->recv_seal_key,
				weak_session_key, recv_seal_const);
		
		dump_data_pw("NTLMSSP recv seal key:\n",
				ntlmssp_state->recv_seal_key, 16);
				
		smb_arc4_init(ntlmssp_state->recv_seal_arc4_state,
				ntlmssp_state->recv_seal_key, 16);

		dump_data_pw("NTLMSSP recv seal arc4 state:\n", 
			     ntlmssp_state->recv_seal_arc4_state, 
			     sizeof(ntlmssp_state->recv_seal_arc4_state));

		ntlmssp_state->ntlm2_send_seq_num = 0;
		ntlmssp_state->ntlm2_recv_seq_num = 0;


	} else {
#if 0
		/* Hmmm. Shouldn't we also weaken keys for ntlmv1 ? JRA. */

		DATA_BLOB weak_session_key = ntlmssp_state->session_key;
		/**
		  Weaken NTLMSSP keys to cope with down-level clients, servers and export restrictions.
		  We probably should have some parameters to control this, once we get NTLM2 working.
		*/

		if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_128) {
			;
		} else if (ntlmssp_state->neg_flags & NTLMSSP_NEGOTIATE_56) {
			weak_session_key.length = 6;
		} else { /* forty bits */
			weak_session_key.length = 5;
		}
		dump_data_pw("NTLMSSP weakend master key:\n",
				weak_session_key.data,
				weak_session_key.length);
#endif

		DATA_BLOB weak_session_key = ntlmssp_weaken_keys(ntlmssp_state, mem_ctx);

		DEBUG(5, ("NTLMSSP Sign/Seal - using NTLM1\n"));

		smb_arc4_init(ntlmssp_state->ntlmv1_arc4_state,
                             weak_session_key.data, weak_session_key.length);

                dump_data_pw("NTLMv1 arc4 state:\n", ntlmssp_state->ntlmv1_arc4_state,
				sizeof(ntlmssp_state->ntlmv1_arc4_state));

		ntlmssp_state->ntlmv1_seq_num = 0;
	}

	TALLOC_FREE(mem_ctx);
	return NT_STATUS_OK;
}

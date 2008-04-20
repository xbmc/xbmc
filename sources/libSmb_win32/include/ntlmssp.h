/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell 1992-1997
   Copyright (C) Luke Kenneth Casson Leighton 1996-1997
   Copyright (C) Paul Ashton 1997
   
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

/* NTLMSSP mode */
enum NTLMSSP_ROLE
{
	NTLMSSP_SERVER,
	NTLMSSP_CLIENT
};

/* NTLMSSP message types */
enum NTLM_MESSAGE_TYPE
{
	NTLMSSP_INITIAL = 0 /* samba internal state */,
	NTLMSSP_NEGOTIATE = 1,
	NTLMSSP_CHALLENGE = 2,
	NTLMSSP_AUTH      = 3,
	NTLMSSP_UNKNOWN   = 4,
	NTLMSSP_DONE      = 5 /* samba final state */
};

/* NTLMSSP negotiation flags */
#define NTLMSSP_NEGOTIATE_UNICODE          0x00000001
#define NTLMSSP_NEGOTIATE_OEM              0x00000002
#define NTLMSSP_REQUEST_TARGET             0x00000004
#define NTLMSSP_NEGOTIATE_SIGN             0x00000010 /* Message integrity */
#define NTLMSSP_NEGOTIATE_SEAL             0x00000020 /* Message confidentiality */
#define NTLMSSP_NEGOTIATE_DATAGRAM_STYLE   0x00000040
#define NTLMSSP_NEGOTIATE_LM_KEY           0x00000080
#define NTLMSSP_NEGOTIATE_NETWARE          0x00000100
#define NTLMSSP_NEGOTIATE_NTLM             0x00000200
#define NTLMSSP_NEGOTIATE_DOMAIN_SUPPLIED  0x00001000
#define NTLMSSP_NEGOTIATE_WORKSTATION_SUPPLIED 0x00002000
#define NTLMSSP_NEGOTIATE_THIS_IS_LOCAL_CALL  0x00004000
#define NTLMSSP_NEGOTIATE_ALWAYS_SIGN      0x00008000
#define NTLMSSP_TARGET_TYPE_DOMAIN            0x10000
#define NTLMSSP_TARGET_TYPE_SERVER            0x20000
#define NTLMSSP_CHAL_INIT_RESPONSE         0x00010000

#define NTLMSSP_CHAL_ACCEPT_RESPONSE       0x00020000
#define NTLMSSP_CHAL_NON_NT_SESSION_KEY    0x00040000
#define NTLMSSP_NEGOTIATE_NTLM2            0x00080000
#define NTLMSSP_CHAL_TARGET_INFO           0x00800000
#define NTLMSSP_UNKNOWN_02000000	   0x02000000
#define NTLMSSP_NEGOTIATE_128              0x20000000 /* 128-bit encryption */
#define NTLMSSP_NEGOTIATE_KEY_EXCH         0x40000000
#define NTLMSSP_NEGOTIATE_56               0x80000000

#define NTLMSSP_NAME_TYPE_SERVER      0x01
#define NTLMSSP_NAME_TYPE_DOMAIN      0x02
#define NTLMSSP_NAME_TYPE_SERVER_DNS  0x03
#define NTLMSSP_NAME_TYPE_DOMAIN_DNS  0x04

#define NTLMSSP_SIG_SIZE 16

typedef struct ntlmssp_state 
{
	TALLOC_CTX *mem_ctx;
	unsigned int ref_count;
	enum NTLMSSP_ROLE role;
	enum server_types server_role;
	uint32 expected_state;

	BOOL unicode;
	BOOL use_ntlmv2;
	char *user;
	char *domain;
	char *workstation;
	char *password;
	char *server_domain;

	DATA_BLOB internal_chal; /* Random challenge as supplied to the client for NTLM authentication */

	DATA_BLOB chal; /* Random challenge as input into the actual NTLM (or NTLM2) authentication */
 	DATA_BLOB lm_resp;
	DATA_BLOB nt_resp;
	DATA_BLOB session_key;
	
	uint32 neg_flags; /* the current state of negotiation with the NTLMSSP partner */
	
	void *auth_context;

	/**
	 * Callback to get the 'challenge' used for NTLM authentication.  
	 *
	 * @param ntlmssp_state This structure
	 * @return 8 bytes of challnege data, determined by the server to be the challenge for NTLM authentication
	 *
	 */
	const uint8 *(*get_challenge)(const struct ntlmssp_state *ntlmssp_state);

	/**
	 * Callback to find if the challenge used by NTLM authentication may be modified 
	 *
	 * The NTLM2 authentication scheme modifies the effective challenge, but this is not compatiable with the
	 * current 'security=server' implementation..  
	 *
	 * @param ntlmssp_state This structure
	 * @return Can the challenge be set to arbitary values?
	 *
	 */
	BOOL (*may_set_challenge)(const struct ntlmssp_state *ntlmssp_state);

	/**
	 * Callback to set the 'challenge' used for NTLM authentication.  
	 *
	 * The callback may use the void *auth_context to store state information, but the same value is always available
	 * from the DATA_BLOB chal on this structure.
	 *
	 * @param ntlmssp_state This structure
	 * @param challenge 8 bytes of data, agreed by the client and server to be the effective challenge for NTLM2 authentication
	 *
	 */
	NTSTATUS (*set_challenge)(struct ntlmssp_state *ntlmssp_state, DATA_BLOB *challenge);

	/**
	 * Callback to check the user's password.  
	 *
	 * The callback must reads the feilds of this structure for the information it needs on the user 
	 * @param ntlmssp_state This structure
	 * @param nt_session_key If an NT session key is returned by the authentication process, return it here
	 * @param lm_session_key If an LM session key is returned by the authentication process, return it here
	 *
	 */
	NTSTATUS (*check_password)(struct ntlmssp_state *ntlmssp_state, DATA_BLOB *nt_session_key, DATA_BLOB *lm_session_key);

	const char *(*get_global_myname)(void);
	const char *(*get_domain)(void);

	/* ntlmv2 */

	unsigned char send_sign_key[16];
	unsigned char send_seal_key[16];
	unsigned char recv_sign_key[16];
	unsigned char recv_seal_key[16];

	unsigned char send_seal_arc4_state[258];
	unsigned char recv_seal_arc4_state[258];

	uint32 ntlm2_send_seq_num;
	uint32 ntlm2_recv_seq_num;

	/* ntlmv1 */
	unsigned char ntlmv1_arc4_state[258];
	uint32 ntlmv1_seq_num;

	/* it turns out that we don't always get the
	   response in at the time we want to process it.
	   Store it here, until we need it */
	DATA_BLOB stored_response; 
	
} NTLMSSP_STATE;

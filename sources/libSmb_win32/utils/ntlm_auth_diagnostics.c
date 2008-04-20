/* 
   Unix SMB/CIFS implementation.

   Winbind status program.

   Copyright (C) Tim Potter      2000-2003
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2003-2004
   Copyright (C) Francesco Chemolli <kinkie@kame.usr.dsi.unimi.it> 2000 

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
#include "utils/ntlm_auth.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

enum ntlm_break {
	BREAK_NONE,
	BREAK_LM,
	BREAK_NT,
	NO_LM,
	NO_NT
};

/* 
   Authenticate a user with a challenge/response, checking session key
   and valid authentication types
*/

/* 
 * Test the normal 'LM and NTLM' combination
 */

static BOOL test_lm_ntlm_broken(enum ntlm_break break_which) 
{
	BOOL pass = True;
	NTSTATUS nt_status;
	uint32 flags = 0;
	DATA_BLOB lm_response = data_blob(NULL, 24);
	DATA_BLOB nt_response = data_blob(NULL, 24);
	DATA_BLOB session_key = data_blob(NULL, 16);

	uchar lm_key[8];
	uchar user_session_key[16];
	uchar lm_hash[16];
	uchar nt_hash[16];
	DATA_BLOB chall = get_challenge();
	char *error_string;
	
	ZERO_STRUCT(lm_key);
	ZERO_STRUCT(user_session_key);

	flags |= WBFLAG_PAM_LMKEY;
	flags |= WBFLAG_PAM_USER_SESSION_KEY;

	SMBencrypt(opt_password,chall.data,lm_response.data);
	E_deshash(opt_password, lm_hash); 

	SMBNTencrypt(opt_password,chall.data,nt_response.data);

	E_md4hash(opt_password, nt_hash);
	SMBsesskeygen_ntv1(nt_hash, NULL, session_key.data);

	switch (break_which) {
	case BREAK_NONE:
		break;
	case BREAK_LM:
		lm_response.data[0]++;
		break;
	case BREAK_NT:
		nt_response.data[0]++;
		break;
	case NO_LM:
		data_blob_free(&lm_response);
		break;
	case NO_NT:
		data_blob_free(&nt_response);
		break;
	}

	nt_status = contact_winbind_auth_crap(opt_username, opt_domain, 
					      opt_workstation,
					      &chall,
					      &lm_response,
					      &nt_response,
					      flags,
					      lm_key, 
					      user_session_key,
					      &error_string, NULL);
	
	data_blob_free(&lm_response);

	if (!NT_STATUS_IS_OK(nt_status)) {
		d_printf("%s (0x%x)\n", 
			 error_string,
			 NT_STATUS_V(nt_status));
		SAFE_FREE(error_string);
		return break_which == BREAK_NT;
	}

	if (memcmp(lm_hash, lm_key, 
		   sizeof(lm_key)) != 0) {
		DEBUG(1, ("LM Key does not match expectations!\n"));
 		DEBUG(1, ("lm_key:\n"));
		dump_data(1, (const char *)lm_key, 8);
		DEBUG(1, ("expected:\n"));
		dump_data(1, (const char *)lm_hash, 8);
		pass = False;
	}

	if (break_which == NO_NT) {
		if (memcmp(lm_hash, user_session_key, 
			   8) != 0) {
			DEBUG(1, ("NT Session Key does not match expectations (should be LM hash)!\n"));
			DEBUG(1, ("user_session_key:\n"));
			dump_data(1, (const char *)user_session_key, sizeof(user_session_key));
			DEBUG(1, ("expected:\n"));
			dump_data(1, (const char *)lm_hash, sizeof(lm_hash));
			pass = False;
		}
	} else {		
		if (memcmp(session_key.data, user_session_key, 
			   sizeof(user_session_key)) != 0) {
			DEBUG(1, ("NT Session Key does not match expectations!\n"));
			DEBUG(1, ("user_session_key:\n"));
			dump_data(1, (const char *)user_session_key, 16);
			DEBUG(1, ("expected:\n"));
			dump_data(1, (const char *)session_key.data, session_key.length);
			pass = False;
		}
	}
        return pass;
}

/* 
 * Test LM authentication, no NT response supplied
 */

static BOOL test_lm(void) 
{

	return test_lm_ntlm_broken(NO_NT);
}

/* 
 * Test the NTLM response only, no LM.
 */

static BOOL test_ntlm(void) 
{
	return test_lm_ntlm_broken(NO_LM);
}

/* 
 * Test the NTLM response only, but in the LM field.
 */

static BOOL test_ntlm_in_lm(void) 
{
	BOOL pass = True;
	NTSTATUS nt_status;
	uint32 flags = 0;
	DATA_BLOB nt_response = data_blob(NULL, 24);

	uchar lm_key[8];
	uchar lm_hash[16];
	uchar user_session_key[16];
	DATA_BLOB chall = get_challenge();
	char *error_string;
	
	ZERO_STRUCT(user_session_key);

	flags |= WBFLAG_PAM_LMKEY;
	flags |= WBFLAG_PAM_USER_SESSION_KEY;

	SMBNTencrypt(opt_password,chall.data,nt_response.data);

	E_deshash(opt_password, lm_hash); 

	nt_status = contact_winbind_auth_crap(opt_username, opt_domain, 
					      opt_workstation,
					      &chall,
					      &nt_response,
					      NULL,
					      flags,
					      lm_key,
					      user_session_key,
					      &error_string, NULL);
	
	data_blob_free(&nt_response);

	if (!NT_STATUS_IS_OK(nt_status)) {
		d_printf("%s (0x%x)\n", 
			 error_string,
			 NT_STATUS_V(nt_status));
		SAFE_FREE(error_string);
		return False;
	}

	if (memcmp(lm_hash, lm_key, 
		   sizeof(lm_key)) != 0) {
		DEBUG(1, ("LM Key does not match expectations!\n"));
 		DEBUG(1, ("lm_key:\n"));
		dump_data(1, (const char *)lm_key, 8);
		DEBUG(1, ("expected:\n"));
		dump_data(1, (const char *)lm_hash, 8);
		pass = False;
	}
	if (memcmp(lm_hash, user_session_key, 8) != 0) {
		DEBUG(1, ("Session Key (first 8 lm hash) does not match expectations!\n"));
 		DEBUG(1, ("user_session_key:\n"));
		dump_data(1, (const char *)user_session_key, 16);
 		DEBUG(1, ("expected:\n"));
		dump_data(1, (const char *)lm_hash, 8);
		pass = False;
	}
        return pass;
}

/* 
 * Test the NTLM response only, but in the both the NT and LM fields.
 */

static BOOL test_ntlm_in_both(void) 
{
	BOOL pass = True;
	NTSTATUS nt_status;
	uint32 flags = 0;
	DATA_BLOB nt_response = data_blob(NULL, 24);
	DATA_BLOB session_key = data_blob(NULL, 16);

	char lm_key[8];
	char lm_hash[16];
	char user_session_key[16];
	char nt_hash[16];
	DATA_BLOB chall = get_challenge();
	char *error_string;
	
	ZERO_STRUCT(lm_key);
	ZERO_STRUCT(user_session_key);

	flags |= WBFLAG_PAM_LMKEY;
	flags |= WBFLAG_PAM_USER_SESSION_KEY;

	SMBNTencrypt(opt_password,chall.data,nt_response.data);
	E_md4hash(opt_password, (unsigned char *)nt_hash);
	SMBsesskeygen_ntv1((const unsigned char *)nt_hash, NULL, session_key.data);

	E_deshash(opt_password, (unsigned char *)lm_hash); 

	nt_status = contact_winbind_auth_crap(opt_username, opt_domain, 
					      opt_workstation,
					      &chall,
					      &nt_response,
					      &nt_response,
					      flags,
					      (unsigned char *)lm_key,
					      (unsigned char *)user_session_key,
					      &error_string, NULL);
	
	data_blob_free(&nt_response);

	if (!NT_STATUS_IS_OK(nt_status)) {
		d_printf("%s (0x%x)\n", 
			 error_string,
			 NT_STATUS_V(nt_status));
		SAFE_FREE(error_string);
		return False;
	}

	if (memcmp(lm_hash, lm_key, 
		   sizeof(lm_key)) != 0) {
		DEBUG(1, ("LM Key does not match expectations!\n"));
 		DEBUG(1, ("lm_key:\n"));
		dump_data(1, lm_key, 8);
		DEBUG(1, ("expected:\n"));
		dump_data(1, lm_hash, 8);
		pass = False;
	}
	if (memcmp(session_key.data, user_session_key, 
		   sizeof(user_session_key)) != 0) {
		DEBUG(1, ("NT Session Key does not match expectations!\n"));
 		DEBUG(1, ("user_session_key:\n"));
		dump_data(1, user_session_key, 16);
 		DEBUG(1, ("expected:\n"));
		dump_data(1, (const char *)session_key.data, session_key.length);
		pass = False;
	}


        return pass;
}

/* 
 * Test the NTLMv2 and LMv2 responses
 */

static BOOL test_lmv2_ntlmv2_broken(enum ntlm_break break_which) 
{
	BOOL pass = True;
	NTSTATUS nt_status;
	uint32 flags = 0;
	DATA_BLOB ntlmv2_response = data_blob(NULL, 0);
	DATA_BLOB lmv2_response = data_blob(NULL, 0);
	DATA_BLOB ntlmv2_session_key = data_blob(NULL, 0);
	DATA_BLOB names_blob = NTLMv2_generate_names_blob(get_winbind_netbios_name(), get_winbind_domain());

	uchar user_session_key[16];
	DATA_BLOB chall = get_challenge();
	char *error_string;

	ZERO_STRUCT(user_session_key);
	
	flags |= WBFLAG_PAM_USER_SESSION_KEY;

	if (!SMBNTLMv2encrypt(opt_username, opt_domain, opt_password, &chall,
			      &names_blob,
			      &lmv2_response, &ntlmv2_response, 
			      &ntlmv2_session_key)) {
		data_blob_free(&names_blob);
		return False;
	}
	data_blob_free(&names_blob);

	switch (break_which) {
	case BREAK_NONE:
		break;
	case BREAK_LM:
		lmv2_response.data[0]++;
		break;
	case BREAK_NT:
		ntlmv2_response.data[0]++;
		break;
	case NO_LM:
		data_blob_free(&lmv2_response);
		break;
	case NO_NT:
		data_blob_free(&ntlmv2_response);
		break;
	}

	nt_status = contact_winbind_auth_crap(opt_username, opt_domain, 
					      opt_workstation,
					      &chall,
					      &lmv2_response,
					      &ntlmv2_response,
					      flags,
					      NULL, 
					      user_session_key,
					      &error_string, NULL);
	
	data_blob_free(&lmv2_response);
	data_blob_free(&ntlmv2_response);

	if (!NT_STATUS_IS_OK(nt_status)) {
		d_printf("%s (0x%x)\n", 
			 error_string,
			 NT_STATUS_V(nt_status));
		SAFE_FREE(error_string);
		return break_which == BREAK_NT;
	}

	if (break_which != NO_NT && break_which != BREAK_NT && memcmp(ntlmv2_session_key.data, user_session_key, 
		   sizeof(user_session_key)) != 0) {
		DEBUG(1, ("USER (NTLMv2) Session Key does not match expectations!\n"));
 		DEBUG(1, ("user_session_key:\n"));
		dump_data(1, (const char *)user_session_key, 16);
 		DEBUG(1, ("expected:\n"));
		dump_data(1, (const char *)ntlmv2_session_key.data, ntlmv2_session_key.length);
		pass = False;
	}
        return pass;
}

/* 
 * Test the NTLMv2 and LMv2 responses
 */

static BOOL test_lmv2_ntlmv2(void) 
{
	return test_lmv2_ntlmv2_broken(BREAK_NONE);
}

/* 
 * Test the LMv2 response only
 */

static BOOL test_lmv2(void) 
{
	return test_lmv2_ntlmv2_broken(NO_NT);
}

/* 
 * Test the NTLMv2 response only
 */

static BOOL test_ntlmv2(void) 
{
	return test_lmv2_ntlmv2_broken(NO_LM);
}

static BOOL test_lm_ntlm(void) 
{
	return test_lm_ntlm_broken(BREAK_NONE);
}

static BOOL test_ntlm_lm_broken(void) 
{
	return test_lm_ntlm_broken(BREAK_LM);
}

static BOOL test_ntlm_ntlm_broken(void) 
{
	return test_lm_ntlm_broken(BREAK_NT);
}

static BOOL test_ntlmv2_lmv2_broken(void) 
{
	return test_lmv2_ntlmv2_broken(BREAK_LM);
}

static BOOL test_ntlmv2_ntlmv2_broken(void) 
{
	return test_lmv2_ntlmv2_broken(BREAK_NT);
}

static BOOL test_plaintext(enum ntlm_break break_which)
{
	NTSTATUS nt_status;
	uint32 flags = 0;
	DATA_BLOB nt_response = data_blob(NULL, 0);
	DATA_BLOB lm_response = data_blob(NULL, 0);
	char *password;

	uchar user_session_key[16];
	uchar lm_key[16];
	static const uchar zeros[8];
	DATA_BLOB chall = data_blob(zeros, sizeof(zeros));
	char *error_string;

	ZERO_STRUCT(user_session_key);
	
	flags |= WBFLAG_PAM_LMKEY;
	flags |= WBFLAG_PAM_USER_SESSION_KEY;

	if ((push_ucs2_allocate((smb_ucs2_t **)&nt_response.data, opt_password)) == -1) {
		DEBUG(0, ("push_ucs2_allocate failed!\n"));
		exit(1);
	}

	nt_response.length = strlen_w(((void *)nt_response.data))*sizeof(smb_ucs2_t);

	if ((password = strdup_upper(opt_password)) == NULL) {
		DEBUG(0, ("strdup_upper failed!\n"));
		exit(1);
	}

	if ((convert_string_allocate(NULL, CH_UNIX, 
				     CH_DOS, password,
				     strlen(password)+1, 
				     (void**)&lm_response.data,True)) == -1) {
		DEBUG(0, ("convert_string_allocate failed!\n"));
		exit(1);
	}

	SAFE_FREE(password);

	lm_response.length = strlen((const char *)lm_response.data);

	switch (break_which) {
	case BREAK_NONE:
		break;
	case BREAK_LM:
		lm_response.data[0]++;
		break;
	case BREAK_NT:
		nt_response.data[0]++;
		break;
	case NO_LM:
		SAFE_FREE(lm_response.data);
		lm_response.length = 0;
		break;
	case NO_NT:
		SAFE_FREE(nt_response.data);
		nt_response.length = 0;
		break;
	}

	nt_status = contact_winbind_auth_crap(opt_username, opt_domain, 
					      opt_workstation,
					      &chall,
					      &lm_response,
					      &nt_response,
					      flags,
					      lm_key,
					      user_session_key,
					      &error_string, NULL);
	
	SAFE_FREE(nt_response.data);
	SAFE_FREE(lm_response.data);
	data_blob_free(&chall);

	if (!NT_STATUS_IS_OK(nt_status)) {
		d_printf("%s (0x%x)\n", 
			 error_string,
			 NT_STATUS_V(nt_status));
		SAFE_FREE(error_string);
		return break_which == BREAK_NT;
	}

        return break_which != BREAK_NT;
}

static BOOL test_plaintext_none_broken(void) {
	return test_plaintext(BREAK_NONE);
}

static BOOL test_plaintext_lm_broken(void) {
	return test_plaintext(BREAK_LM);
}

static BOOL test_plaintext_nt_broken(void) {
	return test_plaintext(BREAK_NT);
}

static BOOL test_plaintext_nt_only(void) {
	return test_plaintext(NO_LM);
}

static BOOL test_plaintext_lm_only(void) {
	return test_plaintext(NO_NT);
}

/* 
   Tests:
   
   - LM only
   - NT and LM		   
   - NT
   - NT in LM field
   - NT in both fields
   - NTLMv2
   - NTLMv2 and LMv2
   - LMv2
   - plaintext tests (in challenge-response feilds)
  
   check we get the correct session key in each case
   check what values we get for the LM session key
   
*/

static const struct ntlm_tests {
	BOOL (*fn)(void);
	const char *name;
} test_table[] = {
	{test_lm, "LM"},
	{test_lm_ntlm, "LM and NTLM"},
	{test_ntlm, "NTLM"},
	{test_ntlm_in_lm, "NTLM in LM"},
	{test_ntlm_in_both, "NTLM in both"},
	{test_ntlmv2, "NTLMv2"},
	{test_lmv2_ntlmv2, "NTLMv2 and LMv2"},
	{test_lmv2, "LMv2"},
	{test_ntlmv2_lmv2_broken, "NTLMv2 and LMv2, LMv2 broken"},
	{test_ntlmv2_ntlmv2_broken, "NTLMv2 and LMv2, NTLMv2 broken"},
	{test_ntlm_lm_broken, "NTLM and LM, LM broken"},
	{test_ntlm_ntlm_broken, "NTLM and LM, NTLM broken"},
	{test_plaintext_none_broken, "Plaintext"},
	{test_plaintext_lm_broken, "Plaintext LM broken"},
	{test_plaintext_nt_broken, "Plaintext NT broken"},
	{test_plaintext_nt_only, "Plaintext NT only"},
	{test_plaintext_lm_only, "Plaintext LM only"},
	{NULL, NULL}
};

BOOL diagnose_ntlm_auth(void)
{
	unsigned int i;
	BOOL pass = True;

	for (i=0; test_table[i].fn; i++) {
		if (!test_table[i].fn()) {
			DEBUG(1, ("Test %s failed!\n", test_table[i].name));
			pass = False;
		}
	}

        return pass;
}


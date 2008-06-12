/* 
   Unix SMB/CIFS implementation.
   Password and authentication handling
   Copyright (C) Andrew Tridgell              1992-2000
   Copyright (C) Luke Kenneth Casson Leighton 1996-2000
   Copyright (C) Andrew Bartlett              2001-2003
   Copyright (C) Gerald Carter                2003
   
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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

/****************************************************************************
 Core of smb password checking routine.
****************************************************************************/

static BOOL smb_pwd_check_ntlmv1(const DATA_BLOB *nt_response,
				 const uchar *part_passwd,
				 const DATA_BLOB *sec_blob,
				 DATA_BLOB *user_sess_key)
{
	/* Finish the encryption of part_passwd. */
	uchar p24[24];
	
	if (part_passwd == NULL) {
		DEBUG(10,("No password set - DISALLOWING access\n"));
		/* No password set - always false ! */
		return False;
	}
	
	if (sec_blob->length != 8) {
		DEBUG(0, ("smb_pwd_check_ntlmv1: incorrect challenge size (%lu)\n", 
			  (unsigned long)sec_blob->length));
		return False;
	}
	
	if (nt_response->length != 24) {
		DEBUG(0, ("smb_pwd_check_ntlmv1: incorrect password length (%lu)\n", 
			  (unsigned long)nt_response->length));
		return False;
	}

	SMBOWFencrypt(part_passwd, sec_blob->data, p24);
	if (user_sess_key != NULL) {
		*user_sess_key = data_blob(NULL, 16);
		SMBsesskeygen_ntv1(part_passwd, NULL, user_sess_key->data);
	}
	
	
#ifdef DEBUG_PASSWORD
	DEBUG(100,("Part password (P16) was |\n"));
	dump_data(100, (const char *)part_passwd, 16);
	DEBUGADD(100,("Password from client was |\n"));
	dump_data(100, (const char *)nt_response->data, nt_response->length);
	DEBUGADD(100,("Given challenge was |\n"));
	dump_data(100, (const char *)sec_blob->data, sec_blob->length);
	DEBUGADD(100,("Value from encryption was |\n"));
	dump_data(100, (const char *)p24, 24);
#endif
	return (memcmp(p24, nt_response->data, 24) == 0);
}

/****************************************************************************
 Core of smb password checking routine. (NTLMv2, LMv2)
 Note:  The same code works with both NTLMv2 and LMv2.
****************************************************************************/

static BOOL smb_pwd_check_ntlmv2(const DATA_BLOB *ntv2_response,
				 const uchar *part_passwd,
				 const DATA_BLOB *sec_blob,
				 const char *user, const char *domain,
				 BOOL upper_case_domain, /* should the domain be transformed into upper case? */
				 DATA_BLOB *user_sess_key)
{
	/* Finish the encryption of part_passwd. */
	uchar kr[16];
	uchar value_from_encryption[16];
	uchar client_response[16];
	DATA_BLOB client_key_data;
	BOOL res;

	if (part_passwd == NULL) {
		DEBUG(10,("No password set - DISALLOWING access\n"));
		/* No password set - always False */
		return False;
	}

	if (sec_blob->length != 8) {
		DEBUG(0, ("smb_pwd_check_ntlmv2: incorrect challenge size (%lu)\n", 
			  (unsigned long)sec_blob->length));
		return False;
	}
	
	if (ntv2_response->length < 24) {
		/* We MUST have more than 16 bytes, or the stuff below will go
		   crazy.  No known implementation sends less than the 24 bytes
		   for LMv2, let alone NTLMv2. */
		DEBUG(0, ("smb_pwd_check_ntlmv2: incorrect password length (%lu)\n", 
			  (unsigned long)ntv2_response->length));
		return False;
	}

	client_key_data = data_blob(ntv2_response->data+16, ntv2_response->length-16);
	/* 
	   todo:  should we be checking this for anything?  We can't for LMv2, 
	   but for NTLMv2 it is meant to contain the current time etc.
	*/

	memcpy(client_response, ntv2_response->data, sizeof(client_response));

	if (!ntv2_owf_gen(part_passwd, user, domain, upper_case_domain, kr)) {
		return False;
	}

	SMBOWFencrypt_ntv2(kr, sec_blob, &client_key_data, value_from_encryption);
	if (user_sess_key != NULL) {
		*user_sess_key = data_blob(NULL, 16);
		SMBsesskeygen_ntv2(kr, value_from_encryption, user_sess_key->data);
	}

#if DEBUG_PASSWORD
	DEBUG(100,("Part password (P16) was |\n"));
	dump_data(100, (const char *)part_passwd, 16);
	DEBUGADD(100,("Password from client was |\n"));
	dump_data(100, (const char *)ntv2_response->data, ntv2_response->length);
	DEBUGADD(100,("Variable data from client was |\n"));
	dump_data(100, (const char *)client_key_data.data, client_key_data.length);
	DEBUGADD(100,("Given challenge was |\n"));
	dump_data(100, (const char *)sec_blob->data, sec_blob->length);
	DEBUGADD(100,("Value from encryption was |\n"));
	dump_data(100, (const char *)value_from_encryption, 16);
#endif
	data_blob_clear_free(&client_key_data);
	res = (memcmp(value_from_encryption, client_response, 16) == 0);
	if ((!res) && (user_sess_key != NULL))
		data_blob_clear_free(user_sess_key);
	return res;
}

/**
 * Check a challenge-response password against the value of the NT or
 * LM password hash.
 *
 * @param mem_ctx talloc context
 * @param challenge 8-byte challenge.  If all zero, forces plaintext comparison
 * @param nt_response 'unicode' NT response to the challenge, or unicode password
 * @param lm_response ASCII or LANMAN response to the challenge, or password in DOS code page
 * @param username internal Samba username, for log messages
 * @param client_username username the client used
 * @param client_domain domain name the client used (may be mapped)
 * @param nt_pw MD4 unicode password from our passdb or similar
 * @param lm_pw LANMAN ASCII password from our passdb or similar
 * @param user_sess_key User session key
 * @param lm_sess_key LM session key (first 8 bytes of the LM hash)
 */

NTSTATUS ntlm_password_check(TALLOC_CTX *mem_ctx,
			     const DATA_BLOB *challenge,
			     const DATA_BLOB *lm_response,
			     const DATA_BLOB *nt_response,
			     const DATA_BLOB *lm_interactive_pwd,
			     const DATA_BLOB *nt_interactive_pwd,
			     const char *username, 
			     const char *client_username, 
			     const char *client_domain,
			     const uint8 *lm_pw, const uint8 *nt_pw, 
			     DATA_BLOB *user_sess_key, 
			     DATA_BLOB *lm_sess_key)
{
	static const unsigned char zeros[8];
	if (nt_pw == NULL) {
		DEBUG(3,("ntlm_password_check: NO NT password stored for user %s.\n", 
			 username));
	}

	if (nt_interactive_pwd && nt_interactive_pwd->length && nt_pw) { 
		if (nt_interactive_pwd->length != 16) {
			DEBUG(3,("ntlm_password_check: Interactive logon: Invalid NT password length (%d) supplied for user %s\n", (int)nt_interactive_pwd->length,
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

		if (memcmp(nt_interactive_pwd->data, nt_pw, 16) == 0) {
			if (user_sess_key) {
				*user_sess_key = data_blob(NULL, 16);
				SMBsesskeygen_ntv1(nt_pw, NULL, user_sess_key->data);
			}
			return NT_STATUS_OK;
		} else {
			DEBUG(3,("ntlm_password_check: Interactive logon: NT password check failed for user %s\n",
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

	} else if (lm_interactive_pwd && lm_interactive_pwd->length && lm_pw) { 
		if (lm_interactive_pwd->length != 16) {
			DEBUG(3,("ntlm_password_check: Interactive logon: Invalid LANMAN password length (%d) supplied for user %s\n", (int)lm_interactive_pwd->length,
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

		if (!lp_lanman_auth()) {
			DEBUG(3,("ntlm_password_check: Interactive logon: only LANMAN password supplied for user %s, and LM passwords are disabled!\n",
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}

		if (memcmp(lm_interactive_pwd->data, lm_pw, 16) == 0) {
			return NT_STATUS_OK;
		} else {
			DEBUG(3,("ntlm_password_check: Interactive logon: LANMAN password check failed for user %s\n",
				 username));
			return NT_STATUS_WRONG_PASSWORD;
		}
	}

	/* Check for cleartext netlogon. Used by Exchange 5.5. */
	if (challenge->length == sizeof(zeros) && 
	    (memcmp(challenge->data, zeros, challenge->length) == 0 )) {

		DEBUG(4,("ntlm_password_check: checking plaintext passwords for user %s\n",
			 username));
		if (nt_pw && nt_response->length) {
			unsigned char pwhash[16];
			mdfour(pwhash, nt_response->data, nt_response->length);
			if (memcmp(pwhash, nt_pw, sizeof(pwhash)) == 0) {
				return NT_STATUS_OK;
			} else {
				DEBUG(3,("ntlm_password_check: NT (Unicode) plaintext password check failed for user %s\n",
					 username));
				return NT_STATUS_WRONG_PASSWORD;
			}

		} else if (!lp_lanman_auth()) {
			DEBUG(3,("ntlm_password_check: (plaintext password check) LANMAN passwords NOT PERMITTED for user %s\n",
				 username));

		} else if (lm_pw && lm_response->length) {
			uchar dospwd[14]; 
			uchar p16[16]; 
			ZERO_STRUCT(dospwd);
			
			memcpy(dospwd, lm_response->data, MIN(lm_response->length, sizeof(dospwd)));
			/* Only the fisrt 14 chars are considered, password need not be null terminated. */

			/* we *might* need to upper-case the string here */
			E_P16((const unsigned char *)dospwd, p16);

			if (memcmp(p16, lm_pw, sizeof(p16)) == 0) {
				return NT_STATUS_OK;
			} else {
				DEBUG(3,("ntlm_password_check: LANMAN (ASCII) plaintext password check failed for user %s\n",
					 username));
				return NT_STATUS_WRONG_PASSWORD;
			}
		} else {
			DEBUG(3, ("Plaintext authentication for user %s attempted, but neither NT nor LM passwords available\n", username));
			return NT_STATUS_WRONG_PASSWORD;
		}
	}

	if (nt_response->length != 0 && nt_response->length < 24) {
		DEBUG(2,("ntlm_password_check: invalid NT password length (%lu) for user %s\n", 
			 (unsigned long)nt_response->length, username));		
	}
	
	if (nt_response->length >= 24 && nt_pw) {
		if (nt_response->length > 24) {
			/* We have the NT MD4 hash challenge available - see if we can
			   use it 
			*/
			DEBUG(4,("ntlm_password_check: Checking NTLMv2 password with domain [%s]\n", client_domain));
			if (smb_pwd_check_ntlmv2( nt_response, 
						  nt_pw, challenge, 
						  client_username, 
						  client_domain,
						  False,
						  user_sess_key)) {
				return NT_STATUS_OK;
			}
			
			DEBUG(4,("ntlm_password_check: Checking NTLMv2 password with uppercased version of domain [%s]\n", client_domain));
			if (smb_pwd_check_ntlmv2( nt_response, 
						  nt_pw, challenge, 
						  client_username, 
						  client_domain,
						  True,
						  user_sess_key)) {
				return NT_STATUS_OK;
			}
			
			DEBUG(4,("ntlm_password_check: Checking NTLMv2 password without a domain\n"));
			if (smb_pwd_check_ntlmv2( nt_response, 
						  nt_pw, challenge, 
						  client_username, 
						  "",
						  False,
						  user_sess_key)) {
				return NT_STATUS_OK;
			} else {
				DEBUG(3,("ntlm_password_check: NTLMv2 password check failed\n"));
				return NT_STATUS_WRONG_PASSWORD;
			}
		}

		if (lp_ntlm_auth()) {		
			/* We have the NT MD4 hash challenge available - see if we can
			   use it (ie. does it exist in the smbpasswd file).
			*/
			DEBUG(4,("ntlm_password_check: Checking NT MD4 password\n"));
			if (smb_pwd_check_ntlmv1(nt_response, 
						 nt_pw, challenge,
						 user_sess_key)) {
				/* The LM session key for this response is not very secure, 
				   so use it only if we otherwise allow LM authentication */

				if (lp_lanman_auth() && lm_pw) {
					uint8 first_8_lm_hash[16];
					memcpy(first_8_lm_hash, lm_pw, 8);
					memset(first_8_lm_hash + 8, '\0', 8);
					if (lm_sess_key) {
						*lm_sess_key = data_blob(first_8_lm_hash, 16);
					}
				}
				return NT_STATUS_OK;
			} else {
				DEBUG(3,("ntlm_password_check: NT MD4 password check failed for user %s\n",
					 username));
				return NT_STATUS_WRONG_PASSWORD;
			}
		} else {
			DEBUG(2,("ntlm_password_check: NTLMv1 passwords NOT PERMITTED for user %s\n",
				 username));			
			/* no return, becouse we might pick up LMv2 in the LM field */
		}
	}
	
	if (lm_response->length == 0) {
		DEBUG(3,("ntlm_password_check: NEITHER LanMan nor NT password supplied for user %s\n",
			 username));
		return NT_STATUS_WRONG_PASSWORD;
	}
	
	if (lm_response->length < 24) {
		DEBUG(2,("ntlm_password_check: invalid LanMan password length (%lu) for user %s\n", 
			 (unsigned long)nt_response->length, username));		
		return NT_STATUS_WRONG_PASSWORD;
	}
		
	if (!lp_lanman_auth()) {
		DEBUG(3,("ntlm_password_check: Lanman passwords NOT PERMITTED for user %s\n",
			 username));
	} else if (!lm_pw) {
		DEBUG(3,("ntlm_password_check: NO LanMan password set for user %s (and no NT password supplied)\n",
			 username));
	} else {
		DEBUG(4,("ntlm_password_check: Checking LM password\n"));
		if (smb_pwd_check_ntlmv1(lm_response, 
					 lm_pw, challenge,
					 NULL)) {
			uint8 first_8_lm_hash[16];
			memcpy(first_8_lm_hash, lm_pw, 8);
			memset(first_8_lm_hash + 8, '\0', 8);
			if (user_sess_key) {
				*user_sess_key = data_blob(first_8_lm_hash, 16);
			}

			if (lm_sess_key) {
				*lm_sess_key = data_blob(first_8_lm_hash, 16);
			}
			return NT_STATUS_OK;
		}
	}
	
	if (!nt_pw) {
		DEBUG(4,("ntlm_password_check: LM password check failed for user, no NT password %s\n",username));
		return NT_STATUS_WRONG_PASSWORD;
	}
	
	/* This is for 'LMv2' authentication.  almost NTLMv2 but limited to 24 bytes.
	   - related to Win9X, legacy NAS pass-though authentication
	*/
	DEBUG(4,("ntlm_password_check: Checking LMv2 password with domain %s\n", client_domain));
	if (smb_pwd_check_ntlmv2( lm_response, 
				  nt_pw, challenge, 
				  client_username,
				  client_domain,
				  False,
				  NULL)) {
		return NT_STATUS_OK;
	}
	
	DEBUG(4,("ntlm_password_check: Checking LMv2 password with upper-cased version of domain %s\n", client_domain));
	if (smb_pwd_check_ntlmv2( lm_response, 
				  nt_pw, challenge, 
				  client_username,
				  client_domain,
				  True,
				  NULL)) {
		return NT_STATUS_OK;
	}
	
	DEBUG(4,("ntlm_password_check: Checking LMv2 password without a domain\n"));
	if (smb_pwd_check_ntlmv2( lm_response, 
				  nt_pw, challenge, 
				  client_username,
				  "",
				  False,
				  NULL)) {
		return NT_STATUS_OK;
	}

	/* Apparently NT accepts NT responses in the LM field
	   - I think this is related to Win9X pass-though authentication
	*/
	DEBUG(4,("ntlm_password_check: Checking NT MD4 password in LM field\n"));
	if (lp_ntlm_auth()) {
		if (smb_pwd_check_ntlmv1(lm_response, 
					 nt_pw, challenge,
					 NULL)) {
			/* The session key for this response is still very odd.  
			   It not very secure, so use it only if we otherwise 
			   allow LM authentication */

			if (lp_lanman_auth() && lm_pw) {
				uint8 first_8_lm_hash[16];
				memcpy(first_8_lm_hash, lm_pw, 8);
				memset(first_8_lm_hash + 8, '\0', 8);
				if (user_sess_key) {
					*user_sess_key = data_blob(first_8_lm_hash, 16);
				}

				if (lm_sess_key) {
					*lm_sess_key = data_blob(first_8_lm_hash, 16);
				}
			}
			return NT_STATUS_OK;
		}
		DEBUG(3,("ntlm_password_check: LM password, NT MD4 password in LM field and LMv2 failed for user %s\n",username));
	} else {
		DEBUG(3,("ntlm_password_check: LM password and LMv2 failed for user %s, and NT MD4 password in LM field not permitted\n",username));
	}
	return NT_STATUS_WRONG_PASSWORD;
}


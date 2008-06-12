/* 
   Unix SMB/CIFS implementation.
   SMB parameters and setup
   Copyright (C) Andrew Tridgell 1992-1998
   Modified by Jeremy Allison 1995.
   Copyright (C) Jeremy Allison 1995-2000.
   Copyright (C) Luke Kennethc Casson Leighton 1996-2000.
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2002-2003
   
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
#include "byteorder.h"

/*
   This implements the X/Open SMB password encryption
   It takes a password ('unix' string), a 8 byte "crypt key" 
   and puts 24 bytes of encrypted password into p24 

   Returns False if password must have been truncated to create LM hash
*/
BOOL SMBencrypt(const char *passwd, const uchar *c8, uchar p24[24])
{
	BOOL ret;
	uchar p21[21];

	memset(p21,'\0',21);
	ret = E_deshash(passwd, p21); 

	SMBOWFencrypt(p21, c8, p24);

#ifdef DEBUG_PASSWORD
	DEBUG(100,("SMBencrypt: lm#, challenge, response\n"));
	dump_data(100, (const char *)p21, 16);
	dump_data(100, (const char *)c8, 8);
	dump_data(100, (const char *)p24, 24);
#endif

	return ret;
}

/**
 * Creates the MD4 Hash of the users password in NT UNICODE.
 * @param passwd password in 'unix' charset.
 * @param p16 return password hashed with md4, caller allocated 16 byte buffer
 */
 
void E_md4hash(const char *passwd, uchar p16[16])
{
	int len;
	smb_ucs2_t wpwd[129];
	
	/* Password must be converted to NT unicode - null terminated. */
	push_ucs2(NULL, wpwd, (const char *)passwd, 256, STR_UNICODE|STR_NOALIGN|STR_TERMINATE);
	/* Calculate length in bytes */
	len = strlen_w(wpwd) * sizeof(int16);

	mdfour(p16, (unsigned char *)wpwd, len);
	ZERO_STRUCT(wpwd);	
}

/**
 * Creates the MD5 Hash of a combination of 16 byte salt and 16 byte NT hash.
 * @param 16 byte salt.
 * @param 16 byte NT hash.
 * @param 16 byte return hashed with md5, caller allocated 16 byte buffer
 */

void E_md5hash(const uchar salt[16], const uchar nthash[16], uchar hash_out[16])
{
	struct MD5Context tctx;
	uchar array[32];
	
	memset(hash_out, '\0', 16);
	memcpy(array, salt, 16);
	memcpy(&array[16], nthash, 16);
	MD5Init(&tctx);
	MD5Update(&tctx, array, 32);
	MD5Final(hash_out, &tctx);
}

/**
 * Creates the DES forward-only Hash of the users password in DOS ASCII charset
 * @param passwd password in 'unix' charset.
 * @param p16 return password hashed with DES, caller allocated 16 byte buffer
 * @return False if password was > 14 characters, and therefore may be incorrect, otherwise True
 * @note p16 is filled in regardless
 */
 
BOOL E_deshash(const char *passwd, uchar p16[16])
{
	BOOL ret = True;
	fstring dospwd; 
	ZERO_STRUCT(dospwd);
	
	/* Password must be converted to DOS charset - null terminated, uppercase. */
	push_ascii(dospwd, passwd, sizeof(dospwd), STR_UPPER|STR_TERMINATE);
       
	/* Only the fisrt 14 chars are considered, password need not be null terminated. */
	E_P16((const unsigned char *)dospwd, p16);

	if (strlen(dospwd) > 14) {
		ret = False;
	}

	ZERO_STRUCT(dospwd);	

	return ret;
}

/**
 * Creates the MD4 and DES (LM) Hash of the users password.  
 * MD4 is of the NT Unicode, DES is of the DOS UPPERCASE password.
 * @param passwd password in 'unix' charset.
 * @param nt_p16 return password hashed with md4, caller allocated 16 byte buffer
 * @param p16 return password hashed with des, caller allocated 16 byte buffer
 */
 
/* Does both the NT and LM owfs of a user's password */
void nt_lm_owf_gen(const char *pwd, uchar nt_p16[16], uchar p16[16])
{
	/* Calculate the MD4 hash (NT compatible) of the password */
	memset(nt_p16, '\0', 16);
	E_md4hash(pwd, nt_p16);

#ifdef DEBUG_PASSWORD
	DEBUG(100,("nt_lm_owf_gen: pwd, nt#\n"));
	dump_data(120, pwd, strlen(pwd));
	dump_data(100, (char *)nt_p16, 16);
#endif

	E_deshash(pwd, (uchar *)p16);

#ifdef DEBUG_PASSWORD
	DEBUG(100,("nt_lm_owf_gen: pwd, lm#\n"));
	dump_data(120, pwd, strlen(pwd));
	dump_data(100, (char *)p16, 16);
#endif
}

/* Does both the NTLMv2 owfs of a user's password */
BOOL ntv2_owf_gen(const uchar owf[16],
		  const char *user_in, const char *domain_in,
		  BOOL upper_case_domain, /* Transform the domain into UPPER case */
		  uchar kr_buf[16])
{
	smb_ucs2_t *user;
	smb_ucs2_t *domain;
	
	size_t user_byte_len;
	size_t domain_byte_len;

	HMACMD5Context ctx;

	user_byte_len = push_ucs2_allocate(&user, user_in);
	if (user_byte_len == (size_t)-1) {
		DEBUG(0, ("push_uss2_allocate() for user returned -1 (probably malloc() failure)\n"));
		return False;
	}

	domain_byte_len = push_ucs2_allocate(&domain, domain_in);
	if (domain_byte_len == (size_t)-1) {
		DEBUG(0, ("push_uss2_allocate() for domain returned -1 (probably malloc() failure)\n"));
		return False;
	}

	strupper_w(user);

	if (upper_case_domain)
		strupper_w(domain);

	SMB_ASSERT(user_byte_len >= 2);
	SMB_ASSERT(domain_byte_len >= 2);

	/* We don't want null termination */
	user_byte_len = user_byte_len - 2;
	domain_byte_len = domain_byte_len - 2;
	
	hmac_md5_init_limK_to_64(owf, 16, &ctx);
	hmac_md5_update((const unsigned char *)user, user_byte_len, &ctx);
	hmac_md5_update((const unsigned char *)domain, domain_byte_len, &ctx);
	hmac_md5_final(kr_buf, &ctx);

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("ntv2_owf_gen: user, domain, owfkey, kr\n"));
	dump_data(100, (const char *)user, user_byte_len);
	dump_data(100, (const char *)domain, domain_byte_len);
	dump_data(100, (const char *)owf, 16);
	dump_data(100, (const char *)kr_buf, 16);
#endif

	SAFE_FREE(user);
	SAFE_FREE(domain);
	return True;
}

/* Does the des encryption from the NT or LM MD4 hash. */
void SMBOWFencrypt(const uchar passwd[16], const uchar *c8, uchar p24[24])
{
	uchar p21[21];

	ZERO_STRUCT(p21);
 
	memcpy(p21, passwd, 16);    
	E_P24(p21, c8, p24);
}

/* Does the des encryption from the FIRST 8 BYTES of the NT or LM MD4 hash. */
void NTLMSSPOWFencrypt(const uchar passwd[8], const uchar *ntlmchalresp, uchar p24[24])
{
	uchar p21[21];
 
	memset(p21,'\0',21);
	memcpy(p21, passwd, 8);    
	memset(p21 + 8, 0xbd, 8);    

	E_P24(p21, ntlmchalresp, p24);
#ifdef DEBUG_PASSWORD
	DEBUG(100,("NTLMSSPOWFencrypt: p21, c8, p24\n"));
	dump_data(100, (char *)p21, 21);
	dump_data(100, (const char *)ntlmchalresp, 8);
	dump_data(100, (char *)p24, 24);
#endif
}


/* Does the NT MD4 hash then des encryption. */
 
void SMBNTencrypt(const char *passwd, uchar *c8, uchar *p24)
{
	uchar p21[21];
 
	memset(p21,'\0',21);
 
	E_md4hash(passwd, p21);    
	SMBOWFencrypt(p21, c8, p24);

#ifdef DEBUG_PASSWORD
	DEBUG(100,("SMBNTencrypt: nt#, challenge, response\n"));
	dump_data(100, (char *)p21, 16);
	dump_data(100, (char *)c8, 8);
	dump_data(100, (char *)p24, 24);
#endif
}

/* Does the md5 encryption from the Key Response for NTLMv2. */
void SMBOWFencrypt_ntv2(const uchar kr[16],
			const DATA_BLOB *srv_chal,
			const DATA_BLOB *cli_chal,
			uchar resp_buf[16])
{
	HMACMD5Context ctx;

	hmac_md5_init_limK_to_64(kr, 16, &ctx);
	hmac_md5_update(srv_chal->data, srv_chal->length, &ctx);
	hmac_md5_update(cli_chal->data, cli_chal->length, &ctx);
	hmac_md5_final(resp_buf, &ctx);

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("SMBOWFencrypt_ntv2: srv_chal, cli_chal, resp_buf\n"));
	dump_data(100, (const char *)srv_chal->data, srv_chal->length);
	dump_data(100, (const char *)cli_chal->data, cli_chal->length);
	dump_data(100, (const char *)resp_buf, 16);
#endif
}

void SMBsesskeygen_ntv2(const uchar kr[16],
			const uchar * nt_resp, uint8 sess_key[16])
{
	/* a very nice, 128 bit, variable session key */
	
	HMACMD5Context ctx;

	hmac_md5_init_limK_to_64(kr, 16, &ctx);
	hmac_md5_update(nt_resp, 16, &ctx);
	hmac_md5_final((unsigned char *)sess_key, &ctx);

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("SMBsesskeygen_ntv2:\n"));
	dump_data(100, (const char *)sess_key, 16);
#endif
}

void SMBsesskeygen_ntv1(const uchar kr[16],
			const uchar * nt_resp, uint8 sess_key[16])
{
	/* yes, this session key does not change - yes, this 
	   is a problem - but it is 128 bits */
	
	mdfour((unsigned char *)sess_key, kr, 16);

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("SMBsesskeygen_ntv1:\n"));
	dump_data(100, (const char *)sess_key, 16);
#endif
}

void SMBsesskeygen_lm_sess_key(const uchar lm_hash[16],
			const uchar lm_resp[24], /* only uses 8 */ 
			uint8 sess_key[16])
{
	uchar p24[24];
	uchar partial_lm_hash[16];
	
	memcpy(partial_lm_hash, lm_hash, 8);
	memset(partial_lm_hash + 8, 0xbd, 8);    

	SMBOWFencrypt(partial_lm_hash, lm_resp, p24);
	
	memcpy(sess_key, p24, 16);

#ifdef DEBUG_PASSWORD
	DEBUG(100, ("SMBsesskeygen_lmv1_jerry:\n"));
	dump_data(100, (const char *)sess_key, 16);
#endif
}

DATA_BLOB NTLMv2_generate_names_blob(const char *hostname, 
				     const char *domain)
{
	DATA_BLOB names_blob = data_blob(NULL, 0);
	
	msrpc_gen(&names_blob, "aaa", 
		  NTLMSSP_NAME_TYPE_DOMAIN, domain,
		  NTLMSSP_NAME_TYPE_SERVER, hostname,
		  0, "");
	return names_blob;
}

static DATA_BLOB NTLMv2_generate_client_data(const DATA_BLOB *names_blob) 
{
	uchar client_chal[8];
	DATA_BLOB response = data_blob(NULL, 0);
	char long_date[8];

	generate_random_buffer(client_chal, sizeof(client_chal));

	put_long_date(long_date, time(NULL));

	/* See http://www.ubiqx.org/cifs/SMB.html#SMB.8.5 */

	msrpc_gen(&response, "ddbbdb", 
		  0x00000101,     /* Header  */
		  0,              /* 'Reserved'  */
		  long_date, 8,	  /* Timestamp */
		  client_chal, 8, /* client challenge */
		  0,		  /* Unknown */
		  names_blob->data, names_blob->length);	/* End of name list */

	return response;
}

static DATA_BLOB NTLMv2_generate_response(const uchar ntlm_v2_hash[16],
					  const DATA_BLOB *server_chal,
					  const DATA_BLOB *names_blob)
{
	uchar ntlmv2_response[16];
	DATA_BLOB ntlmv2_client_data;
	DATA_BLOB final_response;
	
	/* NTLMv2 */
	/* generate some data to pass into the response function - including
	   the hostname and domain name of the server */
	ntlmv2_client_data = NTLMv2_generate_client_data(names_blob);

	/* Given that data, and the challenge from the server, generate a response */
	SMBOWFencrypt_ntv2(ntlm_v2_hash, server_chal, &ntlmv2_client_data, ntlmv2_response);
	
	final_response = data_blob(NULL, sizeof(ntlmv2_response) + ntlmv2_client_data.length);

	memcpy(final_response.data, ntlmv2_response, sizeof(ntlmv2_response));

	memcpy(final_response.data+sizeof(ntlmv2_response), 
	       ntlmv2_client_data.data, ntlmv2_client_data.length);

	data_blob_free(&ntlmv2_client_data);

	return final_response;
}

static DATA_BLOB LMv2_generate_response(const uchar ntlm_v2_hash[16],
					const DATA_BLOB *server_chal)
{
	uchar lmv2_response[16];
	DATA_BLOB lmv2_client_data = data_blob(NULL, 8);
	DATA_BLOB final_response = data_blob(NULL, 24);
	
	/* LMv2 */
	/* client-supplied random data */
	generate_random_buffer(lmv2_client_data.data, lmv2_client_data.length);	

	/* Given that data, and the challenge from the server, generate a response */
	SMBOWFencrypt_ntv2(ntlm_v2_hash, server_chal, &lmv2_client_data, lmv2_response);
	memcpy(final_response.data, lmv2_response, sizeof(lmv2_response));

	/* after the first 16 bytes is the random data we generated above, 
	   so the server can verify us with it */
	memcpy(final_response.data+sizeof(lmv2_response), 
	       lmv2_client_data.data, lmv2_client_data.length);

	data_blob_free(&lmv2_client_data);

	return final_response;
}

BOOL SMBNTLMv2encrypt(const char *user, const char *domain, const char *password, 
		      const DATA_BLOB *server_chal, 
		      const DATA_BLOB *names_blob,
		      DATA_BLOB *lm_response, DATA_BLOB *nt_response, 
		      DATA_BLOB *user_session_key) 
{
	uchar nt_hash[16];
	uchar ntlm_v2_hash[16];
	E_md4hash(password, nt_hash);

	/* We don't use the NT# directly.  Instead we use it mashed up with
	   the username and domain.
	   This prevents username swapping during the auth exchange
	*/
	if (!ntv2_owf_gen(nt_hash, user, domain, True, ntlm_v2_hash)) {
		return False;
	}
	
	if (nt_response) {
		*nt_response = NTLMv2_generate_response(ntlm_v2_hash, server_chal,
							names_blob); 
		if (user_session_key) {
			*user_session_key = data_blob(NULL, 16);
			
			/* The NTLMv2 calculations also provide a session key, for signing etc later */
			/* use only the first 16 bytes of nt_response for session key */
			SMBsesskeygen_ntv2(ntlm_v2_hash, nt_response->data, user_session_key->data);
		}
	}
	
	/* LMv2 */
	
	if (lm_response) {
		*lm_response = LMv2_generate_response(ntlm_v2_hash, server_chal);
	}
	
	return True;
}

/***********************************************************
 encode a password buffer with a unicode password.  The buffer
 is filled with random data to make it harder to attack.
************************************************************/
BOOL encode_pw_buffer(uint8 buffer[516], const char *password, int string_flags)
{
	uchar new_pw[512];
	size_t new_pw_len;

	/* the incoming buffer can be any alignment. */
	string_flags |= STR_NOALIGN;

	new_pw_len = push_string(NULL, new_pw,
				 password, 
				 sizeof(new_pw), string_flags);
	
	memcpy(&buffer[512 - new_pw_len], new_pw, new_pw_len);

	generate_random_buffer(buffer, 512 - new_pw_len);

	/* 
	 * The length of the new password is in the last 4 bytes of
	 * the data buffer.
	 */
	SIVAL(buffer, 512, new_pw_len);
	ZERO_STRUCT(new_pw);
	return True;
}


/***********************************************************
 decode a password buffer
 *new_pw_len is the length in bytes of the possibly mulitbyte
 returned password including termination.
************************************************************/

BOOL decode_pw_buffer(uint8 in_buffer[516], char *new_pwrd,
		      int new_pwrd_size, uint32 *new_pw_len,
		      int string_flags)
{
	int byte_len=0;

	/* the incoming buffer can be any alignment. */
	string_flags |= STR_NOALIGN;

	/*
	  Warning !!! : This function is called from some rpc call.
	  The password IN the buffer may be a UNICODE string.
	  The password IN new_pwrd is an ASCII string
	  If you reuse that code somewhere else check first.
	*/

	/* The length of the new password is in the last 4 bytes of the data buffer. */

	byte_len = IVAL(in_buffer, 512);

#ifdef DEBUG_PASSWORD
	dump_data(100, (const char *)in_buffer, 516);
#endif

	/* Password cannot be longer than the size of the password buffer */
	if ( (byte_len < 0) || (byte_len > 512)) {
		DEBUG(0, ("decode_pw_buffer: incorrect password length (%d).\n", byte_len));
		DEBUG(0, ("decode_pw_buffer: check that 'encrypt passwords = yes'\n"));
		return False;
	}

	/* decode into the return buffer.  Buffer length supplied */
 	*new_pw_len = pull_string(NULL, new_pwrd, &in_buffer[512 - byte_len], new_pwrd_size, 
				  byte_len, string_flags);

#ifdef DEBUG_PASSWORD
	DEBUG(100,("decode_pw_buffer: new_pwrd: "));
	dump_data(100, (const char *)new_pwrd, *new_pw_len);
	DEBUG(100,("multibyte len:%d\n", *new_pw_len));
	DEBUG(100,("original char len:%d\n", byte_len/2));
#endif
	
	return True;
}

/***********************************************************
 Decode an arc4 encrypted password change buffer.
************************************************************/

void encode_or_decode_arc4_passwd_buffer(unsigned char pw_buf[532], const DATA_BLOB *psession_key)
{
	struct MD5Context tctx;
	unsigned char key_out[16];

	/* Confounder is last 16 bytes. */

	MD5Init(&tctx);
	MD5Update(&tctx, &pw_buf[516], 16);
	MD5Update(&tctx, psession_key->data, psession_key->length);
	MD5Final(key_out, &tctx);
	/* arc4 with key_out. */
	SamOEMhash(pw_buf, key_out, 516);
}

/***********************************************************
 Encrypt/Decrypt used for LSA secrets and trusted domain
 passwords.
************************************************************/

void sess_crypt_blob(DATA_BLOB *out, const DATA_BLOB *in, const DATA_BLOB *session_key, int forward)
{
	int i, k;

	for (i=0,k=0;
	     i<in->length;
	     i += 8, k += 7) {
		uint8 bin[8], bout[8], key[7];

		memset(bin, 0, 8);
		memcpy(bin,  &in->data[i], MIN(8, in->length-i));

		if (k + 7 > session_key->length) {
			k = (session_key->length - k);
		}
		memcpy(key, &session_key->data[k], 7);

		des_crypt56(bout, bin, key, forward?1:0);

		memcpy(&out->data[i], bout, MIN(8, in->length-i));
        }
}

/* Decrypts password-blob with session-key
 * @param pass		password for session-key
 * @param data_in 	DATA_BLOB encrypted password
 *
 * Returns cleartext password in CH_UNIX 
 * Caller must free the returned string
 */

char *decrypt_trustdom_secret(const char *pass, DATA_BLOB *data_in)
{
	DATA_BLOB data_out, sess_key;
	uchar nt_hash[16];
	uint32_t length;
	uint32_t version;
	fstring cleartextpwd;

	if (!data_in || !pass)
		return NULL;

	/* generate md4 password-hash derived from the NT UNICODE password */
	E_md4hash(pass, nt_hash);

	/* hashed twice with md4 */
	mdfour(nt_hash, nt_hash, 16);

	/* 16-Byte session-key */
	sess_key = data_blob(nt_hash, 16);
	if (sess_key.data == NULL)
		return NULL;
	
	data_out = data_blob(NULL, data_in->length);
	if (data_out.data == NULL)
		return NULL;
	
	/* decrypt with des3 */
	sess_crypt_blob(&data_out, data_in, &sess_key, 0);

	/* 4 Byte length, 4 Byte version */
	length  = IVAL(data_out.data, 0);
	version = IVAL(data_out.data, 4);

	if (length > data_in->length - 8) {
		DEBUG(0,("decrypt_trustdom_secret: invalid length (%d)\n", length));
		return NULL;
	}
	
	if (version != 1) {
		DEBUG(0,("decrypt_trustdom_secret: unknown version number (%d)\n", version));
		return NULL;
	}
	
	rpcstr_pull(cleartextpwd, data_out.data + 8, sizeof(fstring), length, 0 );

#ifdef DEBUG_PASSWORD
	DEBUG(100,("decrypt_trustdom_secret: length is: %d, version is: %d, password is: %s\n", 
				length, version, cleartextpwd));
#endif

	data_blob_free(&data_out);
	data_blob_free(&sess_key);
	
	return SMB_STRDUP(cleartextpwd);

}


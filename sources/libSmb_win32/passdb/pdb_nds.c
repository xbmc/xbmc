/* 
   Unix SMB/CIFS mplementation.
   NDS LDAP helper functions for SAMBA
   Copyright (C) Vince Brimhall			2004-2005
    
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

#include <lber.h>
#include <ldap.h>
#include <wchar.h>

#include "smbldap.h"

#define NMASLDAP_GET_LOGIN_CONFIG_REQUEST	"2.16.840.1.113719.1.39.42.100.3"
#define NMASLDAP_GET_LOGIN_CONFIG_RESPONSE	"2.16.840.1.113719.1.39.42.100.4"
#define NMASLDAP_SET_PASSWORD_REQUEST		"2.16.840.1.113719.1.39.42.100.11"
#define NMASLDAP_SET_PASSWORD_RESPONSE		"2.16.840.1.113719.1.39.42.100.12"
#define NMASLDAP_GET_PASSWORD_REQUEST		"2.16.840.1.113719.1.39.42.100.13"
#define NMASLDAP_GET_PASSWORD_RESPONSE		"2.16.840.1.113719.1.39.42.100.14"

#define NMAS_LDAP_EXT_VERSION				1

/**********************************************************************
 Take the request BER value and input data items and BER encodes the
 data into the BER value
**********************************************************************/

static int berEncodePasswordData(
	struct berval **requestBV,
	const char    *objectDN,
	const char    *password,
	const char    *password2)
{
	int err = 0, rc=0;
	BerElement *requestBer = NULL;

	const char    * utf8ObjPtr = NULL;
	int     utf8ObjSize = 0;
	const char    * utf8PwdPtr = NULL;
	int     utf8PwdSize = 0;
	const char    * utf8Pwd2Ptr = NULL;
	int     utf8Pwd2Size = 0;


	/* Convert objectDN and tag strings from Unicode to UTF-8 */
	utf8ObjSize = strlen(objectDN)+1;
	utf8ObjPtr = objectDN;

	if (password != NULL)
	{
		utf8PwdSize = strlen(password)+1;
		utf8PwdPtr = password;
	}

	if (password2 != NULL)
	{
		utf8Pwd2Size = strlen(password2)+1;
		utf8Pwd2Ptr = password2;
	}

	/* Allocate a BerElement for the request parameters. */
	if((requestBer = ber_alloc()) == NULL)
	{
		err = LDAP_ENCODING_ERROR;
		goto Cleanup;
	}

	if (password != NULL && password2 != NULL)
	{
		/* BER encode the NMAS Version, the objectDN, and the password */
		rc = ber_printf(requestBer, "{iooo}", NMAS_LDAP_EXT_VERSION, utf8ObjPtr, utf8ObjSize, utf8PwdPtr, utf8PwdSize, utf8Pwd2Ptr, utf8Pwd2Size);
	}
	else if (password != NULL)
	{
		/* BER encode the NMAS Version, the objectDN, and the password */
		rc = ber_printf(requestBer, "{ioo}", NMAS_LDAP_EXT_VERSION, utf8ObjPtr, utf8ObjSize, utf8PwdPtr, utf8PwdSize);
	}
	else
	{
		/* BER encode the NMAS Version and the objectDN */
		rc = ber_printf(requestBer, "{io}", NMAS_LDAP_EXT_VERSION, utf8ObjPtr, utf8ObjSize);
	}

	if (rc < 0)
	{
		err = LDAP_ENCODING_ERROR;
		goto Cleanup;
	}
	else
	{
		err = 0;
	}

	/* Convert the BER we just built to a berval that we'll send with the extended request. */
	if(ber_flatten(requestBer, requestBV) == LBER_ERROR)
	{
		err = LDAP_ENCODING_ERROR;
		goto Cleanup;
	}

Cleanup:

	if(requestBer)
	{
		ber_free(requestBer, 1);
	}

	return err;
}

/**********************************************************************
 Take the request BER value and input data items and BER encodes the
 data into the BER value
**********************************************************************/

static int berEncodeLoginData(
	struct berval **requestBV,
	char     *objectDN,
	unsigned int  methodIDLen,
	unsigned int *methodID,
	char     *tag,
	size_t   putDataLen,
	void     *putData)
{
	int err = 0;
	BerElement *requestBer = NULL;

	unsigned int i;
	unsigned int elemCnt = methodIDLen / sizeof(unsigned int);

	char	*utf8ObjPtr=NULL;
	int     utf8ObjSize = 0;

	char    *utf8TagPtr = NULL;
	int     utf8TagSize = 0;

	utf8ObjPtr = objectDN;
	utf8ObjSize = strlen(utf8ObjPtr)+1;

	utf8TagPtr = tag;
	utf8TagSize = strlen(utf8TagPtr)+1;

	/* Allocate a BerElement for the request parameters. */
	if((requestBer = ber_alloc()) == NULL)
	{
		err = LDAP_ENCODING_ERROR;
		goto Cleanup;
	}

	/* BER encode the NMAS Version and the objectDN */
	err = (ber_printf(requestBer, "{io", NMAS_LDAP_EXT_VERSION, utf8ObjPtr, utf8ObjSize) < 0) ? LDAP_ENCODING_ERROR : 0;

	/* BER encode the MethodID Length and value */
	if (!err)
	{
		err = (ber_printf(requestBer, "{i{", methodIDLen) < 0) ? LDAP_ENCODING_ERROR : 0;
	}

	for (i = 0; !err && i < elemCnt; i++)
	{
		err = (ber_printf(requestBer, "i", methodID[i]) < 0) ? LDAP_ENCODING_ERROR : 0;
	}

	if (!err)
	{
		err = (ber_printf(requestBer, "}}", 0) < 0) ? LDAP_ENCODING_ERROR : 0;
	}

	if(putData)
	{
		/* BER Encode the the tag and data */
		err = (ber_printf(requestBer, "oio}", utf8TagPtr, utf8TagSize, putDataLen, putData, putDataLen) < 0) ? LDAP_ENCODING_ERROR : 0;
	}
	else
	{
		/* BER Encode the the tag */
		err = (ber_printf(requestBer, "o}", utf8TagPtr, utf8TagSize) < 0) ? LDAP_ENCODING_ERROR : 0;
	}

	if (err)
	{
		goto Cleanup;
	}

	/* Convert the BER we just built to a berval that we'll send with the extended request. */
	if(ber_flatten(requestBer, requestBV) == LBER_ERROR)
	{
		err = LDAP_ENCODING_ERROR;
		goto Cleanup;
	}

Cleanup:

	if(requestBer)
	{
		ber_free(requestBer, 1);
	}

	return err;
}

/**********************************************************************
 Takes the reply BER Value and decodes the NMAS server version and
 return code and if a non null retData buffer was supplied, tries to
 decode the the return data and length
**********************************************************************/

static int berDecodeLoginData(
	struct berval *replyBV,
	int      *serverVersion,
	size_t   *retDataLen,
	void     *retData )
{
	int err = 0;
	BerElement *replyBer = NULL;
	char    *retOctStr = NULL;
	size_t  retOctStrLen = 0;

	if((replyBer = ber_init(replyBV)) == NULL)
	{
		err = LDAP_OPERATIONS_ERROR;
		goto Cleanup;
	}

	if(retData)
	{
		retOctStrLen = *retDataLen + 1;
		retOctStr = SMB_MALLOC(retOctStrLen);
		if(!retOctStr)
		{
			err = LDAP_OPERATIONS_ERROR;
			goto Cleanup;
		}
	
		if(ber_scanf(replyBer, "{iis}", serverVersion, &err, retOctStr, &retOctStrLen) != -1)
		{
			if (*retDataLen >= retOctStrLen)
			{
				memcpy(retData, retOctStr, retOctStrLen);
			}
			else if (!err)
			{	
				err = LDAP_NO_MEMORY;
			}

			*retDataLen = retOctStrLen;
		}
		else if (!err)
		{
			err = LDAP_DECODING_ERROR;
		}
	}
	else
	{
		if(ber_scanf(replyBer, "{ii}", serverVersion, &err) == -1)
		{
			if (!err)
			{
				err = LDAP_DECODING_ERROR;
			}
		}
	}

Cleanup:

	if(replyBer)
	{
		ber_free(replyBer, 1);
	}

	if (retOctStr != NULL)
	{
		memset(retOctStr, 0, retOctStrLen);
		free(retOctStr);
	}

	return err;
}

/**********************************************************************
 Retrieves data in the login configuration of the specified object
 that is tagged with the specified methodID and tag.
**********************************************************************/

static int getLoginConfig(
	LDAP	 *ld,
	char     *objectDN,
	unsigned int  methodIDLen,
	unsigned int *methodID,
	char     *tag,
	size_t   *dataLen,
	void     *data )
{
	int     err = 0;
	struct  berval *requestBV = NULL;
	char    *replyOID = NULL;
	struct  berval *replyBV = NULL;
	int     serverVersion = 0;

	/* Validate unicode parameters. */
	if((strlen(objectDN) == 0) || ld == NULL)
	{
		return LDAP_NO_SUCH_ATTRIBUTE;
	}

	err = berEncodeLoginData(&requestBV, objectDN, methodIDLen, methodID, tag, 0, NULL);
	if(err)
	{
		goto Cleanup;
	}

	/* Call the ldap_extended_operation (synchronously) */
	if((err = ldap_extended_operation_s(ld, NMASLDAP_GET_LOGIN_CONFIG_REQUEST,
					requestBV, NULL, NULL, &replyOID, &replyBV)))
	{
		goto Cleanup;
	}

	/* Make sure there is a return OID */
	if(!replyOID)
	{
		err = LDAP_NOT_SUPPORTED;
		goto Cleanup;
	}

	/* Is this what we were expecting to get back. */
	if(strcmp(replyOID, NMASLDAP_GET_LOGIN_CONFIG_RESPONSE))
	{
		err = LDAP_NOT_SUPPORTED;
		goto Cleanup;
	}

	/* Do we have a good returned berval? */
	if(!replyBV)
	{
		/* No; returned berval means we experienced a rather drastic error. */
		/* Return operations error. */
		err = LDAP_OPERATIONS_ERROR;
		goto Cleanup;
	}

	err = berDecodeLoginData(replyBV, &serverVersion, dataLen, data);

	if(serverVersion != NMAS_LDAP_EXT_VERSION)
	{
		err = LDAP_OPERATIONS_ERROR;
		goto Cleanup;
	}

Cleanup:

	if(replyBV)
	{
		ber_bvfree(replyBV);
	}

	/* Free the return OID string if one was returned. */
	if(replyOID)
	{
		ldap_memfree(replyOID);
	}

	/* Free memory allocated while building the request ber and berval. */
	if(requestBV)
	{
		ber_bvfree(requestBV);
	}

	/* Return the appropriate error/success code. */
	return err;
}

/**********************************************************************
 Attempts to get the Simple Password
**********************************************************************/

static int nmasldap_get_simple_pwd(
	LDAP	 *ld,
	char     *objectDN,
	size_t	 pwdLen,
	char     *pwd )
{
	int err = 0;
	unsigned int methodID = 0;
	unsigned int methodIDLen = sizeof(methodID);
	char    tag[] = {'P','A','S','S','W','O','R','D',' ','H','A','S','H',0};
	char    *pwdBuf=NULL;
	size_t  pwdBufLen, bufferLen;

	bufferLen = pwdBufLen = pwdLen+2;
	pwdBuf = SMB_MALLOC(pwdBufLen); /* digest and null */
	if(pwdBuf == NULL)
	{
		return LDAP_NO_MEMORY;
	}

	err = getLoginConfig(ld, objectDN, methodIDLen, &methodID, tag, &pwdBufLen, pwdBuf);
	if (err == 0)
	{
		if (pwdBufLen !=0)
		{
			pwdBuf[pwdBufLen] = 0;       /* null terminate */

			switch (pwdBuf[0])
			{
				case 1:  /* cleartext password  */
					break;
				case 2:  /* SHA1 HASH */
				case 3:  /* MD5_ID */
				case 4:  /* UNIXCrypt_ID */
				case 8:  /* SSHA_ID */
				default: /* Unknown digest */
					err = LDAP_INAPPROPRIATE_AUTH;  /* only return clear text */
					break;
			}

			if (!err)
			{
				if (pwdLen >= pwdBufLen-1)
				{
					memcpy(pwd, &pwdBuf[1], pwdBufLen-1);  /* skip digest tag and include null */
				}
				else
				{
					err = LDAP_NO_MEMORY;
				}
			}
		}
	}

	if (pwdBuf != NULL)
	{
		memset(pwdBuf, 0, bufferLen);
		free(pwdBuf);
	}

	return err;
}


/**********************************************************************
 Attempts to set the Universal Password
**********************************************************************/

static int nmasldap_set_password(
	LDAP	 *ld,
	const char     *objectDN,
	const char     *pwd )
{
	int err = 0;

	struct berval *requestBV = NULL;
	char *replyOID = NULL;
	struct berval *replyBV = NULL;
	int serverVersion;

	/* Validate char parameters. */
	if(objectDN == NULL || (strlen(objectDN) == 0) || pwd == NULL || ld == NULL)
	{
		return LDAP_NO_SUCH_ATTRIBUTE;
	}

	err = berEncodePasswordData(&requestBV, objectDN, pwd, NULL);
	if(err)
	{
		goto Cleanup;
	}

	/* Call the ldap_extended_operation (synchronously) */
	if((err = ldap_extended_operation_s(ld, NMASLDAP_SET_PASSWORD_REQUEST, requestBV, NULL, NULL, &replyOID, &replyBV)))
	{
		goto Cleanup;
	}

	/* Make sure there is a return OID */
	if(!replyOID)
	{
		err = LDAP_NOT_SUPPORTED;
		goto Cleanup;
	}

	/* Is this what we were expecting to get back. */
	if(strcmp(replyOID, NMASLDAP_SET_PASSWORD_RESPONSE))
	{
		err = LDAP_NOT_SUPPORTED;
		goto Cleanup;
	}

	/* Do we have a good returned berval? */
	if(!replyBV)
	{
		/* No; returned berval means we experienced a rather drastic error. */
		/* Return operations error. */
		err = LDAP_OPERATIONS_ERROR;
		goto Cleanup;
	}

	err = berDecodeLoginData(replyBV, &serverVersion, NULL, NULL);

	if(serverVersion != NMAS_LDAP_EXT_VERSION)
	{
		err = LDAP_OPERATIONS_ERROR;
		goto Cleanup;
	}

Cleanup:

	if(replyBV)
	{
		ber_bvfree(replyBV);
	}

	/* Free the return OID string if one was returned. */
	if(replyOID)
	{
		ldap_memfree(replyOID);
	}

	/* Free memory allocated while building the request ber and berval. */
	if(requestBV)
	{
		ber_bvfree(requestBV);
	}

	/* Return the appropriate error/success code. */
	return err;
}

/**********************************************************************
 Attempts to get the Universal Password
**********************************************************************/

static int nmasldap_get_password(
	LDAP	 *ld,
	char     *objectDN,
	size_t   *pwdSize,	/* in bytes */
	unsigned char     *pwd )
{
	int err = 0;

	struct berval *requestBV = NULL;
	char *replyOID = NULL;
	struct berval *replyBV = NULL;
	int serverVersion;
	char *pwdBuf;
	size_t pwdBufLen, bufferLen;

	/* Validate char parameters. */
	if(objectDN == NULL || (strlen(objectDN) == 0) || pwdSize == NULL || ld == NULL)
	{
		return LDAP_NO_SUCH_ATTRIBUTE;
	}

	bufferLen = pwdBufLen = *pwdSize;
	pwdBuf = SMB_MALLOC(pwdBufLen+2);
	if(pwdBuf == NULL)
	{
		return LDAP_NO_MEMORY;
	}

	err = berEncodePasswordData(&requestBV, objectDN, NULL, NULL);
	if(err)
	{
		goto Cleanup;
	}

	/* Call the ldap_extended_operation (synchronously) */
	if((err = ldap_extended_operation_s(ld, NMASLDAP_GET_PASSWORD_REQUEST, requestBV, NULL, NULL, &replyOID, &replyBV)))
	{
		goto Cleanup;
	}

	/* Make sure there is a return OID */
	if(!replyOID)
	{
		err = LDAP_NOT_SUPPORTED;
		goto Cleanup;
	}

	/* Is this what we were expecting to get back. */
	if(strcmp(replyOID, NMASLDAP_GET_PASSWORD_RESPONSE))
	{
		err = LDAP_NOT_SUPPORTED;
		goto Cleanup;
	}

	/* Do we have a good returned berval? */
	if(!replyBV)
	{
		/* No; returned berval means we experienced a rather drastic error. */
		/* Return operations error. */
		err = LDAP_OPERATIONS_ERROR;
		goto Cleanup;
	}

	err = berDecodeLoginData(replyBV, &serverVersion, &pwdBufLen, pwdBuf);

	if(serverVersion != NMAS_LDAP_EXT_VERSION)
	{
		err = LDAP_OPERATIONS_ERROR;
		goto Cleanup;
	}

	if (!err && pwdBufLen != 0)
	{
		if (*pwdSize >= pwdBufLen+1 && pwd != NULL)
		{
			memcpy(pwd, pwdBuf, pwdBufLen);
			pwd[pwdBufLen] = 0; /* add null termination */
		}
		*pwdSize = pwdBufLen; /* does not include null termination */
	}

Cleanup:

	if(replyBV)
	{
		ber_bvfree(replyBV);
	}

	/* Free the return OID string if one was returned. */
	if(replyOID)
	{
		ldap_memfree(replyOID);
	}

	/* Free memory allocated while building the request ber and berval. */
	if(requestBV)
	{
		ber_bvfree(requestBV);
	}

	if (pwdBuf != NULL)
	{
		memset(pwdBuf, 0, bufferLen);
		free(pwdBuf);
	}

	/* Return the appropriate error/success code. */
	return err;
}

/**********************************************************************
 Get the user's password from NDS.
 *********************************************************************/

int pdb_nds_get_password(
	struct smbldap_state *ldap_state,
	char *object_dn,
	size_t *pwd_len,
	char *pwd )
{
	LDAP *ld = ldap_state->ldap_struct;
	int rc = -1;

	rc = nmasldap_get_password(ld, object_dn, pwd_len, (unsigned char *)pwd);
	if (rc == LDAP_SUCCESS) {
#ifdef DEBUG_PASSWORD
		DEBUG(100,("nmasldap_get_password returned %s for %s\n", pwd, object_dn));
#endif    
		DEBUG(5, ("NDS Universal Password retrieved for %s\n", object_dn));
	} else {
		DEBUG(3, ("NDS Universal Password NOT retrieved for %s\n", object_dn));
	}

	if (rc != LDAP_SUCCESS) {
		rc = nmasldap_get_simple_pwd(ld, object_dn, *pwd_len, pwd);
		if (rc == LDAP_SUCCESS) {
#ifdef DEBUG_PASSWORD
			DEBUG(100,("nmasldap_get_simple_pwd returned %s for %s\n", pwd, object_dn));
#endif    
			DEBUG(5, ("NDS Simple Password retrieved for %s\n", object_dn));
		} else {
			/* We couldn't get the password */
			DEBUG(3, ("NDS Simple Password NOT retrieved for %s\n", object_dn));
			return LDAP_INVALID_CREDENTIALS;
		}
	}

	/* We got the password */
	return LDAP_SUCCESS;
}

/**********************************************************************
 Set the users NDS, Universal and Simple passwords.
 ********************************************************************/

int pdb_nds_set_password(
	struct smbldap_state *ldap_state,
	char *object_dn,
	const char *pwd )
{
	LDAP *ld = ldap_state->ldap_struct;
	int rc = -1;
	LDAPMod **tmpmods = NULL;

	rc = nmasldap_set_password(ld, object_dn, pwd);
	if (rc == LDAP_SUCCESS) {
		DEBUG(5,("NDS Universal Password changed for user %s\n", object_dn));
	} else {
		char *ld_error = NULL;
		ldap_get_option(ld, LDAP_OPT_ERROR_STRING, &ld_error);
		
		/* This will fail if Universal Password is not enabled for the user's context */
		DEBUG(3,("NDS Universal Password could not be changed for user %s: %s (%s)\n",
				 object_dn, ldap_err2string(rc), ld_error?ld_error:"unknown"));
		SAFE_FREE(ld_error);
	}

	/* Set eDirectory Password */
	smbldap_set_mod(&tmpmods, LDAP_MOD_REPLACE, "userPassword", pwd);
	rc = smbldap_modify(ldap_state, object_dn, tmpmods);

	return rc;
}

/**********************************************************************
 Allow ldap server to update internal login attempt counters by
  performing a simple bind. If the samba authentication failed attempt
  the bind with a bogus, randomly generated password to count the
  failed attempt. If the bind fails even though samba authentication
  succeeded, this would indicate that the user's account is disabled,
  time restrictions are in place or some other password policy
  violation.
*********************************************************************/

static NTSTATUS pdb_nds_update_login_attempts(struct pdb_methods *methods,
					struct samu *sam_acct, BOOL success)
{
	struct ldapsam_privates *ldap_state;

	if ((!methods) || (!sam_acct)) {
		DEBUG(3,("pdb_nds_update_login_attempts: invalid parameter.\n"));
		return NT_STATUS_MEMORY_NOT_ALLOCATED;
	}

	ldap_state = (struct ldapsam_privates *)methods->private_data;

	if (ldap_state) {
		/* Attempt simple bind with user credentials to update eDirectory
		   password policy */
		int rc = 0;
		char *dn;
		LDAPMessage *result = NULL;
		LDAPMessage *entry = NULL;
		const char **attr_list;
		size_t pwd_len;
		char clear_text_pw[512];
		LDAP *ld = NULL;
		const char *username = pdb_get_username(sam_acct);
		BOOL got_clear_text_pw = False;

		DEBUG(5,("pdb_nds_update_login_attempts: %s login for %s\n",
				success ? "Successful" : "Failed", username));

		result = pdb_get_backend_private_data(sam_acct, methods);
		if (!result) {
			attr_list = get_userattr_list(NULL,
						      ldap_state->schema_ver);
			rc = ldapsam_search_suffix_by_name(ldap_state, username, &result, attr_list );
			TALLOC_FREE( attr_list );
			if (rc != LDAP_SUCCESS) {
				return NT_STATUS_OBJECT_NAME_NOT_FOUND;
			}
			pdb_set_backend_private_data(sam_acct, result, NULL,
						     methods, PDB_CHANGED);
			talloc_autofree_ldapmsg(sam_acct, result);
		}

		if (ldap_count_entries(ldap_state->smbldap_state->ldap_struct, result) == 0) {
			DEBUG(0, ("pdb_nds_update_login_attempts: No user to modify!\n"));
			return NT_STATUS_OBJECT_NAME_NOT_FOUND;
		}

		entry = ldap_first_entry(ldap_state->smbldap_state->ldap_struct, result);
		dn = smbldap_get_dn(ldap_state->smbldap_state->ldap_struct, entry);
		if (!dn) {
			return NT_STATUS_OBJECT_NAME_NOT_FOUND;
		}

		DEBUG(3, ("pdb_nds_update_login_attempts: username %s found dn '%s'\n", username, dn));

		pwd_len = sizeof(clear_text_pw);
		if (success == True) {
			if (pdb_nds_get_password(ldap_state->smbldap_state, dn, &pwd_len, clear_text_pw) == LDAP_SUCCESS) {
				/* Got clear text password. Use simple ldap bind */
				got_clear_text_pw = True;
			}
		} else {
			generate_random_buffer((unsigned char *)clear_text_pw, 24);
			clear_text_pw[24] = '\0';
			DEBUG(5,("pdb_nds_update_login_attempts: using random password %s\n", clear_text_pw));
		}

		if((success != True) || (got_clear_text_pw == True)) {
			
			rc = smb_ldap_setup_full_conn(&ld, ldap_state->location);
			if (rc) {
				return NT_STATUS_INVALID_CONNECTION;
			}

			/* Attempt simple bind with real or bogus password */
			rc = ldap_simple_bind_s(ld, dn, clear_text_pw);
			ldap_unbind(ld);
			if (rc == LDAP_SUCCESS) {
				DEBUG(5,("pdb_nds_update_login_attempts: ldap_simple_bind_s Successful for %s\n", username));
			} else {
				NTSTATUS nt_status = NT_STATUS_ACCOUNT_RESTRICTION;
				DEBUG(5,("pdb_nds_update_login_attempts: ldap_simple_bind_s Failed for %s\n", username));
				switch(rc) {
					case LDAP_INVALID_CREDENTIALS:
						nt_status = NT_STATUS_WRONG_PASSWORD;
						break;
					case LDAP_UNWILLING_TO_PERFORM:
						/* eDir returns this if the account was disabled. */
						/* The problem is we don't know if the given
						   password was correct for this account or
						   not. We have to return more info than we
						   should and tell the client NT_STATUS_ACCOUNT_DISABLED
						   so they don't think the password was bad. JRA. */
						nt_status = NT_STATUS_ACCOUNT_DISABLED;
						break;
					default:
						break;
				}
				return nt_status;
			}
		}
	}
	
	return NT_STATUS_OK;
}

/**********************************************************************
 Intitalise the parts of the pdb_methods structuire that are common 
 to NDS_ldapsam modes
 *********************************************************************/

static NTSTATUS pdb_init_NDS_ldapsam_common(struct pdb_methods **pdb_method, const char *location)
{
	struct ldapsam_privates *ldap_state = (*pdb_method)->private_data;

	/* Mark this as eDirectory ldap */
	ldap_state->is_nds_ldap = True;

	/* Add pdb_nds specific method for updating login attempts. */
	(*pdb_method)->update_login_attempts = pdb_nds_update_login_attempts;

	/* Save location for use in pdb_nds_update_login_attempts */
	ldap_state->location = SMB_STRDUP(location);

	return NT_STATUS_OK;
}


/**********************************************************************
 Initialise the 'nds compat' mode for pdb_ldap
 *********************************************************************/

static NTSTATUS pdb_init_NDS_ldapsam_compat(struct pdb_methods **pdb_method, const char *location)
{
	NTSTATUS nt_status = pdb_init_ldapsam_compat(pdb_method, location);

	(*pdb_method)->name = "NDS_ldapsam_compat";

	pdb_init_NDS_ldapsam_common(pdb_method, location);

	return nt_status;
}


/**********************************************************************
 Initialise the 'nds' normal mode for pdb_ldap
 *********************************************************************/

static NTSTATUS pdb_init_NDS_ldapsam(struct pdb_methods **pdb_method, const char *location)
{
	NTSTATUS nt_status = pdb_init_ldapsam(pdb_method, location);

	(*pdb_method)->name = "NDS_ldapsam";

	pdb_init_NDS_ldapsam_common(pdb_method, location);

	return nt_status;
}

NTSTATUS pdb_nds_init(void)
{
	NTSTATUS nt_status;
	if (!NT_STATUS_IS_OK(nt_status = smb_register_passdb(PASSDB_INTERFACE_VERSION, "NDS_ldapsam", pdb_init_NDS_ldapsam)))
		return nt_status;

	if (!NT_STATUS_IS_OK(nt_status = smb_register_passdb(PASSDB_INTERFACE_VERSION, "NDS_ldapsam_compat", pdb_init_NDS_ldapsam_compat)))
		return nt_status;

	return NT_STATUS_OK;
}

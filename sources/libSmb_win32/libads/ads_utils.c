/* 
   Unix SMB/CIFS implementation.
   ads (active directory) utility library
   
   Copyright (C) Stefan (metze) Metzmacher 2002
   Copyright (C) Andrew Tridgell 2001
  
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

/* 
translated the ACB_CTRL Flags to UserFlags (userAccountControl) 
*/ 
uint32 ads_acb2uf(uint32 acb)
{
	uint32 uf = 0x00000000;
	
	if (acb & ACB_DISABLED) 		uf |= UF_ACCOUNTDISABLE;
	if (acb & ACB_HOMDIRREQ) 		uf |= UF_HOMEDIR_REQUIRED;
	if (acb & ACB_PWNOTREQ) 		uf |= UF_PASSWD_NOTREQD;	
	if (acb & ACB_TEMPDUP) 			uf |= UF_TEMP_DUPLICATE_ACCOUNT;	
	if (acb & ACB_NORMAL)	 		uf |= UF_NORMAL_ACCOUNT;
	if (acb & ACB_MNS) 			uf |= UF_MNS_LOGON_ACCOUNT;
	if (acb & ACB_DOMTRUST) 		uf |= UF_INTERDOMAIN_TRUST_ACCOUNT;
	if (acb & ACB_WSTRUST) 			uf |= UF_WORKSTATION_TRUST_ACCOUNT;
	if (acb & ACB_SVRTRUST) 		uf |= UF_SERVER_TRUST_ACCOUNT;
	if (acb & ACB_PWNOEXP) 			uf |= UF_DONT_EXPIRE_PASSWD;
	if (acb & ACB_AUTOLOCK) 		uf |= UF_LOCKOUT;
	if (acb & ACB_USE_DES_KEY_ONLY)		uf |= UF_USE_DES_KEY_ONLY;
	if (acb & ACB_SMARTCARD_REQUIRED)	uf |= UF_SMARTCARD_REQUIRED;
	if (acb & ACB_TRUSTED_FOR_DELEGATION)	uf |= UF_TRUSTED_FOR_DELEGATION;
	if (acb & ACB_DONT_REQUIRE_PREAUTH)	uf |= UF_DONT_REQUIRE_PREAUTH;
	if (acb & ACB_NO_AUTH_DATA_REQD)	uf |= UF_NO_AUTH_DATA_REQUIRED;
	if (acb & ACB_NOT_DELEGATED)		uf |= UF_NOT_DELEGATED;
	if (acb & ACB_ENC_TXT_PWD_ALLOWED)	uf |= UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED;

	return uf;
}

/*
translated the UserFlags (userAccountControl) to ACB_CTRL Flags
*/
uint32 ads_uf2acb(uint32 uf)
{
	uint32 acb = 0x00000000;
	
	if (uf & UF_ACCOUNTDISABLE) 		acb |= ACB_DISABLED;
	if (uf & UF_HOMEDIR_REQUIRED) 		acb |= ACB_HOMDIRREQ;
	if (uf & UF_PASSWD_NOTREQD) 		acb |= ACB_PWNOTREQ;	
	if (uf & UF_MNS_LOGON_ACCOUNT) 		acb |= ACB_MNS;
	if (uf & UF_DONT_EXPIRE_PASSWD)		acb |= ACB_PWNOEXP;
	if (uf & UF_LOCKOUT) 			acb |= ACB_AUTOLOCK;
	if (uf & UF_USE_DES_KEY_ONLY)		acb |= ACB_USE_DES_KEY_ONLY;
	if (uf & UF_SMARTCARD_REQUIRED)		acb |= ACB_SMARTCARD_REQUIRED;
	if (uf & UF_TRUSTED_FOR_DELEGATION)	acb |= ACB_TRUSTED_FOR_DELEGATION;
	if (uf & UF_DONT_REQUIRE_PREAUTH)	acb |= ACB_DONT_REQUIRE_PREAUTH;
	if (uf & UF_NO_AUTH_DATA_REQUIRED)	acb |= ACB_NO_AUTH_DATA_REQD;
	if (uf & UF_NOT_DELEGATED)		acb |= ACB_NOT_DELEGATED;
	if (uf & UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED) acb |= ACB_ENC_TXT_PWD_ALLOWED;
	
	switch (uf & UF_ACCOUNT_TYPE_MASK)
	{
		case UF_TEMP_DUPLICATE_ACCOUNT:		acb |= ACB_TEMPDUP;break;	
		case UF_NORMAL_ACCOUNT:	 		acb |= ACB_NORMAL;break;
		case UF_INTERDOMAIN_TRUST_ACCOUNT: 	acb |= ACB_DOMTRUST;break;
		case UF_WORKSTATION_TRUST_ACCOUNT:	acb |= ACB_WSTRUST;break;
		case UF_SERVER_TRUST_ACCOUNT: 		acb |= ACB_SVRTRUST;break;
		/*Fix Me: what should we do here? */
		default: 				acb |= ACB_NORMAL;break;
	}

	return acb;
}

/* 
get the accountType from the UserFlags
*/
uint32 ads_uf2atype(uint32 uf)
{
	uint32 atype = 0x00000000;
		
	if (uf & UF_NORMAL_ACCOUNT)			atype = ATYPE_NORMAL_ACCOUNT;
	else if (uf & UF_TEMP_DUPLICATE_ACCOUNT)	atype = ATYPE_NORMAL_ACCOUNT;
	else if (uf & UF_SERVER_TRUST_ACCOUNT)		atype = ATYPE_WORKSTATION_TRUST;
	else if (uf & UF_WORKSTATION_TRUST_ACCOUNT)	atype = ATYPE_WORKSTATION_TRUST;
	else if (uf & UF_INTERDOMAIN_TRUST_ACCOUNT)	atype = ATYPE_INTERDOMAIN_TRUST;

	return atype;
} 

/* 
get the accountType from the groupType
*/
uint32 ads_gtype2atype(uint32 gtype)
{
	uint32 atype = 0x00000000;
	
	switch(gtype) {
		case GTYPE_SECURITY_BUILTIN_LOCAL_GROUP:
			atype = ATYPE_SECURITY_LOCAL_GROUP;
			break;
		case GTYPE_SECURITY_DOMAIN_LOCAL_GROUP:
			atype = ATYPE_SECURITY_LOCAL_GROUP;
			break;
		case GTYPE_SECURITY_GLOBAL_GROUP:
			atype = ATYPE_SECURITY_GLOBAL_GROUP;
			break;
	
		case GTYPE_DISTRIBUTION_GLOBAL_GROUP:
			atype = ATYPE_DISTRIBUTION_GLOBAL_GROUP;
			break;
		case GTYPE_DISTRIBUTION_DOMAIN_LOCAL_GROUP:
			atype = ATYPE_DISTRIBUTION_UNIVERSAL_GROUP;
			break;
		case GTYPE_DISTRIBUTION_UNIVERSAL_GROUP:
			atype = ATYPE_DISTRIBUTION_LOCAL_GROUP;
			break;
	}

	return atype;
}

/* turn a sAMAccountType into a SID_NAME_USE */
enum SID_NAME_USE ads_atype_map(uint32 atype)
{
	switch (atype & 0xF0000000) {
	case ATYPE_GLOBAL_GROUP:
		return SID_NAME_DOM_GRP;
	case ATYPE_SECURITY_LOCAL_GROUP:
		return SID_NAME_ALIAS;
	case ATYPE_ACCOUNT:
		return SID_NAME_USER;
	default:
		DEBUG(1,("hmm, need to map account type 0x%x\n", atype));
	}
	return SID_NAME_UNKNOWN;
}

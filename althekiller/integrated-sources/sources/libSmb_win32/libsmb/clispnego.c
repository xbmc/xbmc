/* 
   Unix SMB/CIFS implementation.
   simple kerberos5/SPNEGO routines
   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Jim McDonough <jmcd@us.ibm.com> 2002
   Copyright (C) Luke Howard     2003
   
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
  generate a negTokenInit packet given a GUID, a list of supported
  OIDs (the mechanisms) and a principal name string 
*/
DATA_BLOB spnego_gen_negTokenInit(char guid[16], 
				  const char *OIDs[], 
				  const char *principal)
{
	int i;
	ASN1_DATA data;
	DATA_BLOB ret;

	memset(&data, 0, sizeof(data));

	asn1_write(&data, guid, 16);
	asn1_push_tag(&data,ASN1_APPLICATION(0));
	asn1_write_OID(&data,OID_SPNEGO);
	asn1_push_tag(&data,ASN1_CONTEXT(0));
	asn1_push_tag(&data,ASN1_SEQUENCE(0));

	asn1_push_tag(&data,ASN1_CONTEXT(0));
	asn1_push_tag(&data,ASN1_SEQUENCE(0));
	for (i=0; OIDs[i]; i++) {
		asn1_write_OID(&data,OIDs[i]);
	}
	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	asn1_push_tag(&data, ASN1_CONTEXT(3));
	asn1_push_tag(&data, ASN1_SEQUENCE(0));
	asn1_push_tag(&data, ASN1_CONTEXT(0));
	asn1_write_GeneralString(&data,principal);
	asn1_pop_tag(&data);
	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	asn1_pop_tag(&data);

	if (data.has_error) {
		DEBUG(1,("Failed to build negTokenInit at offset %d\n", (int)data.ofs));
		asn1_free(&data);
	}

	ret = data_blob(data.data, data.length);
	asn1_free(&data);

	return ret;
}

/*
  Generate a negTokenInit as used by the client side ... It has a mechType
  (OID), and a mechToken (a security blob) ... 

  Really, we need to break out the NTLMSSP stuff as well, because it could be
  raw in the packets!
*/
DATA_BLOB gen_negTokenInit(const char *OID, DATA_BLOB blob)
{
	ASN1_DATA data;
	DATA_BLOB ret;

	memset(&data, 0, sizeof(data));

	asn1_push_tag(&data, ASN1_APPLICATION(0));
	asn1_write_OID(&data,OID_SPNEGO);
	asn1_push_tag(&data, ASN1_CONTEXT(0));
	asn1_push_tag(&data, ASN1_SEQUENCE(0));

	asn1_push_tag(&data, ASN1_CONTEXT(0));
	asn1_push_tag(&data, ASN1_SEQUENCE(0));
	asn1_write_OID(&data, OID);
	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	asn1_push_tag(&data, ASN1_CONTEXT(2));
	asn1_write_OctetString(&data,blob.data,blob.length);
	asn1_pop_tag(&data);

	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	asn1_pop_tag(&data);

	if (data.has_error) {
		DEBUG(1,("Failed to build negTokenInit at offset %d\n", (int)data.ofs));
		asn1_free(&data);
	}

	ret = data_blob(data.data, data.length);
	asn1_free(&data);

	return ret;
}

/*
  parse a negTokenInit packet giving a GUID, a list of supported
  OIDs (the mechanisms) and a principal name string 
*/
BOOL spnego_parse_negTokenInit(DATA_BLOB blob,
			       char *OIDs[ASN1_MAX_OIDS], 
			       char **principal)
{
	int i;
	BOOL ret;
	ASN1_DATA data;

	asn1_load(&data, blob);

	asn1_start_tag(&data,ASN1_APPLICATION(0));
	asn1_check_OID(&data,OID_SPNEGO);
	asn1_start_tag(&data,ASN1_CONTEXT(0));
	asn1_start_tag(&data,ASN1_SEQUENCE(0));

	asn1_start_tag(&data,ASN1_CONTEXT(0));
	asn1_start_tag(&data,ASN1_SEQUENCE(0));
	for (i=0; asn1_tag_remaining(&data) > 0 && i < ASN1_MAX_OIDS-1; i++) {
		char *oid_str = NULL;
		asn1_read_OID(&data,&oid_str);
		OIDs[i] = oid_str;
	}
	OIDs[i] = NULL;
	asn1_end_tag(&data);
	asn1_end_tag(&data);

	*principal = NULL;
	if (asn1_tag_remaining(&data) > 0) {
		asn1_start_tag(&data, ASN1_CONTEXT(3));
		asn1_start_tag(&data, ASN1_SEQUENCE(0));
		asn1_start_tag(&data, ASN1_CONTEXT(0));
		asn1_read_GeneralString(&data,principal);
		asn1_end_tag(&data);
		asn1_end_tag(&data);
		asn1_end_tag(&data);
	}

	asn1_end_tag(&data);
	asn1_end_tag(&data);

	asn1_end_tag(&data);

	ret = !data.has_error;
	if (data.has_error) {
		int j;
		SAFE_FREE(*principal);
		for(j = 0; j < i && j < ASN1_MAX_OIDS-1; j++) {
			SAFE_FREE(OIDs[j]);
		}
	}

	asn1_free(&data);
	return ret;
}

/*
  generate a negTokenTarg packet given a list of OIDs and a security blob
*/
DATA_BLOB gen_negTokenTarg(const char *OIDs[], DATA_BLOB blob)
{
	int i;
	ASN1_DATA data;
	DATA_BLOB ret;

	memset(&data, 0, sizeof(data));

	asn1_push_tag(&data, ASN1_APPLICATION(0));
	asn1_write_OID(&data,OID_SPNEGO);
	asn1_push_tag(&data, ASN1_CONTEXT(0));
	asn1_push_tag(&data, ASN1_SEQUENCE(0));

	asn1_push_tag(&data, ASN1_CONTEXT(0));
	asn1_push_tag(&data, ASN1_SEQUENCE(0));
	for (i=0; OIDs[i]; i++) {
		asn1_write_OID(&data,OIDs[i]);
	}
	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	asn1_push_tag(&data, ASN1_CONTEXT(2));
	asn1_write_OctetString(&data,blob.data,blob.length);
	asn1_pop_tag(&data);

	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	asn1_pop_tag(&data);

	if (data.has_error) {
		DEBUG(1,("Failed to build negTokenTarg at offset %d\n", (int)data.ofs));
		asn1_free(&data);
	}

	ret = data_blob(data.data, data.length);
	asn1_free(&data);

	return ret;
}

/*
  parse a negTokenTarg packet giving a list of OIDs and a security blob
*/
BOOL parse_negTokenTarg(DATA_BLOB blob, char *OIDs[ASN1_MAX_OIDS], DATA_BLOB *secblob)
{
	int i;
	ASN1_DATA data;

	asn1_load(&data, blob);
	asn1_start_tag(&data, ASN1_APPLICATION(0));
	asn1_check_OID(&data,OID_SPNEGO);
	asn1_start_tag(&data, ASN1_CONTEXT(0));
	asn1_start_tag(&data, ASN1_SEQUENCE(0));

	asn1_start_tag(&data, ASN1_CONTEXT(0));
	asn1_start_tag(&data, ASN1_SEQUENCE(0));
	for (i=0; asn1_tag_remaining(&data) > 0 && i < ASN1_MAX_OIDS-1; i++) {
		char *oid_str = NULL;
		asn1_read_OID(&data,&oid_str);
		OIDs[i] = oid_str;
	}
	OIDs[i] = NULL;
	asn1_end_tag(&data);
	asn1_end_tag(&data);

	asn1_start_tag(&data, ASN1_CONTEXT(2));
	asn1_read_OctetString(&data,secblob);
	asn1_end_tag(&data);

	asn1_end_tag(&data);
	asn1_end_tag(&data);

	asn1_end_tag(&data);

	if (data.has_error) {
		int j;
		data_blob_free(secblob);
		for(j = 0; j < i && j < ASN1_MAX_OIDS-1; j++) {
			SAFE_FREE(OIDs[j]);
		}
		DEBUG(1,("Failed to parse negTokenTarg at offset %d\n", (int)data.ofs));
		asn1_free(&data);
		return False;
	}

	asn1_free(&data);
	return True;
}

/*
  generate a krb5 GSS-API wrapper packet given a ticket
*/
DATA_BLOB spnego_gen_krb5_wrap(const DATA_BLOB ticket, const uint8 tok_id[2])
{
	ASN1_DATA data;
	DATA_BLOB ret;

	memset(&data, 0, sizeof(data));

	asn1_push_tag(&data, ASN1_APPLICATION(0));
	asn1_write_OID(&data, OID_KERBEROS5);

	asn1_write(&data, tok_id, 2);
	asn1_write(&data, ticket.data, ticket.length);
	asn1_pop_tag(&data);

	if (data.has_error) {
		DEBUG(1,("Failed to build krb5 wrapper at offset %d\n", (int)data.ofs));
		asn1_free(&data);
	}

	ret = data_blob(data.data, data.length);
	asn1_free(&data);

	return ret;
}

/*
  parse a krb5 GSS-API wrapper packet giving a ticket
*/
BOOL spnego_parse_krb5_wrap(DATA_BLOB blob, DATA_BLOB *ticket, uint8 tok_id[2])
{
	BOOL ret;
	ASN1_DATA data;
	int data_remaining;

	asn1_load(&data, blob);
	asn1_start_tag(&data, ASN1_APPLICATION(0));
	asn1_check_OID(&data, OID_KERBEROS5);

	data_remaining = asn1_tag_remaining(&data);

	if (data_remaining < 3) {
		data.has_error = True;
	} else {
		asn1_read(&data, tok_id, 2);
		data_remaining -= 2;
		*ticket = data_blob(NULL, data_remaining);
		asn1_read(&data, ticket->data, ticket->length);
	}

	asn1_end_tag(&data);

	ret = !data.has_error;

	if (data.has_error) {
		data_blob_free(ticket);
	}

	asn1_free(&data);

	return ret;
}


/* 
   generate a SPNEGO negTokenTarg packet, ready for a EXTENDED_SECURITY
   kerberos session setup 
*/
int spnego_gen_negTokenTarg(const char *principal, int time_offset, 
			    DATA_BLOB *targ, 
			    DATA_BLOB *session_key_krb5, uint32 extra_ap_opts)
{
	int retval;
	DATA_BLOB tkt, tkt_wrapped;
	const char *krb_mechs[] = {OID_KERBEROS5_OLD, OID_NTLMSSP, NULL};

	/* get a kerberos ticket for the service and extract the session key */
	retval = cli_krb5_get_ticket(principal, time_offset,
					&tkt, session_key_krb5, extra_ap_opts, NULL);

	if (retval)
		return retval;

	/* wrap that up in a nice GSS-API wrapping */
	tkt_wrapped = spnego_gen_krb5_wrap(tkt, TOK_ID_KRB_AP_REQ);

	/* and wrap that in a shiny SPNEGO wrapper */
	*targ = gen_negTokenTarg(krb_mechs, tkt_wrapped);

	data_blob_free(&tkt_wrapped);
	data_blob_free(&tkt);

	return retval;
}


/*
  parse a spnego NTLMSSP challenge packet giving two security blobs
*/
BOOL spnego_parse_challenge(const DATA_BLOB blob,
			    DATA_BLOB *chal1, DATA_BLOB *chal2)
{
	BOOL ret;
	ASN1_DATA data;

	ZERO_STRUCTP(chal1);
	ZERO_STRUCTP(chal2);

	asn1_load(&data, blob);
	asn1_start_tag(&data,ASN1_CONTEXT(1));
	asn1_start_tag(&data,ASN1_SEQUENCE(0));

	asn1_start_tag(&data,ASN1_CONTEXT(0));
	asn1_check_enumerated(&data,1);
	asn1_end_tag(&data);

	asn1_start_tag(&data,ASN1_CONTEXT(1));
	asn1_check_OID(&data, OID_NTLMSSP);
	asn1_end_tag(&data);

	asn1_start_tag(&data,ASN1_CONTEXT(2));
	asn1_read_OctetString(&data, chal1);
	asn1_end_tag(&data);

	/* the second challenge is optional (XP doesn't send it) */
	if (asn1_tag_remaining(&data)) {
		asn1_start_tag(&data,ASN1_CONTEXT(3));
		asn1_read_OctetString(&data, chal2);
		asn1_end_tag(&data);
	}

	asn1_end_tag(&data);
	asn1_end_tag(&data);

	ret = !data.has_error;

	if (data.has_error) {
		data_blob_free(chal1);
		data_blob_free(chal2);
	}

	asn1_free(&data);
	return ret;
}


/*
 generate a SPNEGO auth packet. This will contain the encrypted passwords
*/
DATA_BLOB spnego_gen_auth(DATA_BLOB blob)
{
	ASN1_DATA data;
	DATA_BLOB ret;

	memset(&data, 0, sizeof(data));

	asn1_push_tag(&data, ASN1_CONTEXT(1));
	asn1_push_tag(&data, ASN1_SEQUENCE(0));
	asn1_push_tag(&data, ASN1_CONTEXT(2));
	asn1_write_OctetString(&data,blob.data,blob.length);	
	asn1_pop_tag(&data);
	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	ret = data_blob(data.data, data.length);

	asn1_free(&data);

	return ret;
}

/*
 parse a SPNEGO auth packet. This contains the encrypted passwords
*/
BOOL spnego_parse_auth(DATA_BLOB blob, DATA_BLOB *auth)
{
	ASN1_DATA data;

	asn1_load(&data, blob);
	asn1_start_tag(&data, ASN1_CONTEXT(1));
	asn1_start_tag(&data, ASN1_SEQUENCE(0));
	asn1_start_tag(&data, ASN1_CONTEXT(2));
	asn1_read_OctetString(&data,auth);
	asn1_end_tag(&data);
	asn1_end_tag(&data);
	asn1_end_tag(&data);

	if (data.has_error) {
		DEBUG(3,("spnego_parse_auth failed at %d\n", (int)data.ofs));
		data_blob_free(auth);
		asn1_free(&data);
		return False;
	}

	asn1_free(&data);
	return True;
}

/*
  generate a minimal SPNEGO response packet.  Doesn't contain much.
*/
DATA_BLOB spnego_gen_auth_response(DATA_BLOB *reply, NTSTATUS nt_status,
				   const char *mechOID)
{
	ASN1_DATA data;
	DATA_BLOB ret;
	uint8 negResult;

	if (NT_STATUS_IS_OK(nt_status)) {
		negResult = SPNEGO_NEG_RESULT_ACCEPT;
	} else if (NT_STATUS_EQUAL(nt_status, NT_STATUS_MORE_PROCESSING_REQUIRED)) {
		negResult = SPNEGO_NEG_RESULT_INCOMPLETE; 
	} else {
		negResult = SPNEGO_NEG_RESULT_REJECT; 
	}

	ZERO_STRUCT(data);

	asn1_push_tag(&data, ASN1_CONTEXT(1));
	asn1_push_tag(&data, ASN1_SEQUENCE(0));
	asn1_push_tag(&data, ASN1_CONTEXT(0));
	asn1_write_enumerated(&data, negResult);
	asn1_pop_tag(&data);

	if (reply->data != NULL) {
		asn1_push_tag(&data,ASN1_CONTEXT(1));
		asn1_write_OID(&data, mechOID);
		asn1_pop_tag(&data);
		
		asn1_push_tag(&data,ASN1_CONTEXT(2));
		asn1_write_OctetString(&data, reply->data, reply->length);
		asn1_pop_tag(&data);
	}

	asn1_pop_tag(&data);
	asn1_pop_tag(&data);

	ret = data_blob(data.data, data.length);
	asn1_free(&data);
	return ret;
}

/*
 parse a SPNEGO NTLMSSP auth packet. This contains the encrypted passwords
*/
BOOL spnego_parse_auth_response(DATA_BLOB blob, NTSTATUS nt_status, 
				DATA_BLOB *auth)
{
	ASN1_DATA data;
	uint8 negResult;

	if (NT_STATUS_IS_OK(nt_status)) {
		negResult = SPNEGO_NEG_RESULT_ACCEPT;
	} else if (NT_STATUS_EQUAL(nt_status, NT_STATUS_MORE_PROCESSING_REQUIRED)) {
		negResult = SPNEGO_NEG_RESULT_INCOMPLETE;
	} else {
		negResult = SPNEGO_NEG_RESULT_REJECT;
	}

	asn1_load(&data, blob);
	asn1_start_tag(&data, ASN1_CONTEXT(1));
	asn1_start_tag(&data, ASN1_SEQUENCE(0));
	asn1_start_tag(&data, ASN1_CONTEXT(0));
	asn1_check_enumerated(&data, negResult);
	asn1_end_tag(&data);

	if (negResult == SPNEGO_NEG_RESULT_INCOMPLETE) {
		asn1_start_tag(&data,ASN1_CONTEXT(1));
		asn1_check_OID(&data, OID_NTLMSSP);
		asn1_end_tag(&data);
		
		asn1_start_tag(&data,ASN1_CONTEXT(2));
		asn1_read_OctetString(&data, auth);
		asn1_end_tag(&data);
	}

	asn1_end_tag(&data);
	asn1_end_tag(&data);

	if (data.has_error) {
		DEBUG(3,("spnego_parse_auth_response failed at %d\n", (int)data.ofs));
		asn1_free(&data);
		data_blob_free(auth);
		return False;
	}

	asn1_free(&data);
	return True;
}

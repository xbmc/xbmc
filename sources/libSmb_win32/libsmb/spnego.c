/* 
   Unix SMB/CIFS implementation.

   RFC2478 Compliant SPNEGO implementation

   Copyright (C) Jim McDonough <jmcd@us.ibm.com>   2003

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

static BOOL read_negTokenInit(ASN1_DATA *asn1, negTokenInit_t *token)
{
	ZERO_STRUCTP(token);

	asn1_start_tag(asn1, ASN1_CONTEXT(0));
	asn1_start_tag(asn1, ASN1_SEQUENCE(0));

	while (!asn1->has_error && 0 < asn1_tag_remaining(asn1)) {
		int i;

		switch (asn1->data[asn1->ofs]) {
		/* Read mechTypes */
		case ASN1_CONTEXT(0):
			asn1_start_tag(asn1, ASN1_CONTEXT(0));
			asn1_start_tag(asn1, ASN1_SEQUENCE(0));

			token->mechTypes = SMB_MALLOC_P(const char *);
			for (i = 0; !asn1->has_error &&
				     0 < asn1_tag_remaining(asn1); i++) {
				char *p_oid = NULL;
				token->mechTypes = 
					SMB_REALLOC_ARRAY(token->mechTypes, const char *, i + 2);
				if (!token->mechTypes) {
					asn1->has_error = True;
					return False;
				}
				asn1_read_OID(asn1, &p_oid);
				token->mechTypes[i] = p_oid;
			}
			token->mechTypes[i] = NULL;
			
			asn1_end_tag(asn1);
			asn1_end_tag(asn1);
			break;
		/* Read reqFlags */
		case ASN1_CONTEXT(1):
			asn1_start_tag(asn1, ASN1_CONTEXT(1));
			asn1_read_Integer(asn1, &token->reqFlags);
			token->reqFlags |= SPNEGO_REQ_FLAG;
			asn1_end_tag(asn1);
			break;
                /* Read mechToken */
		case ASN1_CONTEXT(2):
			asn1_start_tag(asn1, ASN1_CONTEXT(2));
			asn1_read_OctetString(asn1, &token->mechToken);
			asn1_end_tag(asn1);
			break;
		/* Read mecListMIC */
		case ASN1_CONTEXT(3):
			asn1_start_tag(asn1, ASN1_CONTEXT(3));
			if (asn1->data[asn1->ofs] == ASN1_OCTET_STRING) {
				asn1_read_OctetString(asn1,
						      &token->mechListMIC);
			} else {
				/* RFC 2478 says we have an Octet String here,
				   but W2k sends something different... */
				char *mechListMIC;
				asn1_push_tag(asn1, ASN1_SEQUENCE(0));
				asn1_push_tag(asn1, ASN1_CONTEXT(0));
				asn1_read_GeneralString(asn1, &mechListMIC);
				asn1_pop_tag(asn1);
				asn1_pop_tag(asn1);

				token->mechListMIC =
					data_blob(mechListMIC, strlen(mechListMIC));
				SAFE_FREE(mechListMIC);
			}
			asn1_end_tag(asn1);
			break;
		default:
			asn1->has_error = True;
			break;
		}
	}

	asn1_end_tag(asn1);
	asn1_end_tag(asn1);

	return !asn1->has_error;
}

static BOOL write_negTokenInit(ASN1_DATA *asn1, negTokenInit_t *token)
{
	asn1_push_tag(asn1, ASN1_CONTEXT(0));
	asn1_push_tag(asn1, ASN1_SEQUENCE(0));

	/* Write mechTypes */
	if (token->mechTypes && *token->mechTypes) {
		int i;

		asn1_push_tag(asn1, ASN1_CONTEXT(0));
		asn1_push_tag(asn1, ASN1_SEQUENCE(0));
		for (i = 0; token->mechTypes[i]; i++) {
			asn1_write_OID(asn1, token->mechTypes[i]);
		}
		asn1_pop_tag(asn1);
		asn1_pop_tag(asn1);
	}

	/* write reqFlags */
	if (token->reqFlags & SPNEGO_REQ_FLAG) {
		int flags = token->reqFlags & ~SPNEGO_REQ_FLAG;

		asn1_push_tag(asn1, ASN1_CONTEXT(1));
		asn1_write_Integer(asn1, flags);
		asn1_pop_tag(asn1);
	}

	/* write mechToken */
	if (token->mechToken.data) {
		asn1_push_tag(asn1, ASN1_CONTEXT(2));
		asn1_write_OctetString(asn1, token->mechToken.data,
				       token->mechToken.length);
		asn1_pop_tag(asn1);
	}

	/* write mechListMIC */
	if (token->mechListMIC.data) {
		asn1_push_tag(asn1, ASN1_CONTEXT(3));
#if 0
		/* This is what RFC 2478 says ... */
		asn1_write_OctetString(asn1, token->mechListMIC.data,
				       token->mechListMIC.length);
#else
		/* ... but unfortunately this is what Windows
		   sends/expects */
		asn1_push_tag(asn1, ASN1_SEQUENCE(0));
		asn1_push_tag(asn1, ASN1_CONTEXT(0));
		asn1_push_tag(asn1, ASN1_GENERAL_STRING);
		asn1_write(asn1, token->mechListMIC.data,
			   token->mechListMIC.length);
		asn1_pop_tag(asn1);
		asn1_pop_tag(asn1);
		asn1_pop_tag(asn1);
#endif		
		asn1_pop_tag(asn1);
	}

	asn1_pop_tag(asn1);
	asn1_pop_tag(asn1);

	return !asn1->has_error;
}

static BOOL read_negTokenTarg(ASN1_DATA *asn1, negTokenTarg_t *token)
{
	ZERO_STRUCTP(token);

	asn1_start_tag(asn1, ASN1_CONTEXT(1));
	asn1_start_tag(asn1, ASN1_SEQUENCE(0));

	while (!asn1->has_error && 0 < asn1_tag_remaining(asn1)) {
		switch (asn1->data[asn1->ofs]) {
		case ASN1_CONTEXT(0):
			asn1_start_tag(asn1, ASN1_CONTEXT(0));
			asn1_start_tag(asn1, ASN1_ENUMERATED);
			asn1_read_uint8(asn1, &token->negResult);
			asn1_end_tag(asn1);
			asn1_end_tag(asn1);
			break;
		case ASN1_CONTEXT(1):
			asn1_start_tag(asn1, ASN1_CONTEXT(1));
			asn1_read_OID(asn1, &token->supportedMech);
			asn1_end_tag(asn1);
			break;
		case ASN1_CONTEXT(2):
			asn1_start_tag(asn1, ASN1_CONTEXT(2));
			asn1_read_OctetString(asn1, &token->responseToken);
			asn1_end_tag(asn1);
			break;
		case ASN1_CONTEXT(3):
			asn1_start_tag(asn1, ASN1_CONTEXT(3));
			asn1_read_OctetString(asn1, &token->mechListMIC);
			asn1_end_tag(asn1);
			break;
		default:
			asn1->has_error = True;
			break;
		}
	}

	asn1_end_tag(asn1);
	asn1_end_tag(asn1);

	return !asn1->has_error;
}

static BOOL write_negTokenTarg(ASN1_DATA *asn1, negTokenTarg_t *token)
{
	asn1_push_tag(asn1, ASN1_CONTEXT(1));
	asn1_push_tag(asn1, ASN1_SEQUENCE(0));

	asn1_push_tag(asn1, ASN1_CONTEXT(0));
	asn1_write_enumerated(asn1, token->negResult);
	asn1_pop_tag(asn1);

	if (token->supportedMech) {
		asn1_push_tag(asn1, ASN1_CONTEXT(1));
		asn1_write_OID(asn1, token->supportedMech);
		asn1_pop_tag(asn1);
	}

	if (token->responseToken.data) {
		asn1_push_tag(asn1, ASN1_CONTEXT(2));
		asn1_write_OctetString(asn1, token->responseToken.data,
				       token->responseToken.length);
		asn1_pop_tag(asn1);
	}

	if (token->mechListMIC.data) {
		asn1_push_tag(asn1, ASN1_CONTEXT(3));
		asn1_write_OctetString(asn1, token->mechListMIC.data,
				      token->mechListMIC.length);
		asn1_pop_tag(asn1);
	}

	asn1_pop_tag(asn1);
	asn1_pop_tag(asn1);

	return !asn1->has_error;
}

ssize_t read_spnego_data(DATA_BLOB data, SPNEGO_DATA *token)
{
	ASN1_DATA asn1;
	ssize_t ret = -1;

	ZERO_STRUCTP(token);
	ZERO_STRUCT(asn1);
	asn1_load(&asn1, data);

	switch (asn1.data[asn1.ofs]) {
	case ASN1_APPLICATION(0):
		asn1_start_tag(&asn1, ASN1_APPLICATION(0));
		asn1_check_OID(&asn1, OID_SPNEGO);
		if (read_negTokenInit(&asn1, &token->negTokenInit)) {
			token->type = SPNEGO_NEG_TOKEN_INIT;
		}
		asn1_end_tag(&asn1);
		break;
	case ASN1_CONTEXT(1):
		if (read_negTokenTarg(&asn1, &token->negTokenTarg)) {
			token->type = SPNEGO_NEG_TOKEN_TARG;
		}
		break;
	default:
		break;
	}

	if (!asn1.has_error) ret = asn1.ofs;
	asn1_free(&asn1);

	return ret;
}

ssize_t write_spnego_data(DATA_BLOB *blob, SPNEGO_DATA *spnego)
{
	ASN1_DATA asn1;
	ssize_t ret = -1;

	ZERO_STRUCT(asn1);

	switch (spnego->type) {
	case SPNEGO_NEG_TOKEN_INIT:
		asn1_push_tag(&asn1, ASN1_APPLICATION(0));
		asn1_write_OID(&asn1, OID_SPNEGO);
		write_negTokenInit(&asn1, &spnego->negTokenInit);
		asn1_pop_tag(&asn1);
		break;
	case SPNEGO_NEG_TOKEN_TARG:
		write_negTokenTarg(&asn1, &spnego->negTokenTarg);
		break;
	default:
		asn1.has_error = True;
		break;
	}

	if (!asn1.has_error) {
		*blob = data_blob(asn1.data, asn1.length);
		ret = asn1.ofs;
	}
	asn1_free(&asn1);

	return ret;
}

BOOL free_spnego_data(SPNEGO_DATA *spnego)
{
	BOOL ret = True;

	if (!spnego) goto out;

	switch(spnego->type) {
	case SPNEGO_NEG_TOKEN_INIT:
		if (spnego->negTokenInit.mechTypes) {
			int i;
			for (i = 0; spnego->negTokenInit.mechTypes[i]; i++) {
				free(CONST_DISCARD(char *,spnego->negTokenInit.mechTypes[i]));
			}
			free(spnego->negTokenInit.mechTypes);
		}
		data_blob_free(&spnego->negTokenInit.mechToken);
		data_blob_free(&spnego->negTokenInit.mechListMIC);
		break;
	case SPNEGO_NEG_TOKEN_TARG:
		if (spnego->negTokenTarg.supportedMech) {
			free(spnego->negTokenTarg.supportedMech);
		}
		data_blob_free(&spnego->negTokenTarg.responseToken);
		data_blob_free(&spnego->negTokenTarg.mechListMIC);
		break;
	default:
		ret = False;
		break;
	}
	ZERO_STRUCTP(spnego);
out:
	return ret;
}

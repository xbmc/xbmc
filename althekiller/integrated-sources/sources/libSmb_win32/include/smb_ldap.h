/* 
   Unix SMB/CIFS Implementation.
   LDAP protocol helper functions for SAMBA
   Copyright (C) Volker Lendecke 2004
    
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

#ifndef _SMB_LDAP_H
#define _SMB_LDAP_H

enum ldap_request_tag {
	LDAP_TAG_BindRequest = 0,
	LDAP_TAG_BindResponse = 1,
	LDAP_TAG_UnbindRequest = 2,
	LDAP_TAG_SearchRequest = 3,
	LDAP_TAG_SearchResultEntry = 4,
	LDAP_TAG_SearchResultDone = 5,
	LDAP_TAG_ModifyRequest = 6,
	LDAP_TAG_ModifyResponse = 7,
	LDAP_TAG_AddRequest = 8,
	LDAP_TAG_AddResponse = 9,
	LDAP_TAG_DelRequest = 10,
	LDAP_TAG_DelResponse = 11,
	LDAP_TAG_ModifyDNRequest = 12,
	LDAP_TAG_ModifyDNResponse = 13,
	LDAP_TAG_CompareRequest = 14,
	LDAP_TAG_CompareResponse = 15,
	LDAP_TAG_AbandonRequest = 16,
	LDAP_TAG_SearchResultReference = 19,
	LDAP_TAG_ExtendedRequest = 23,
	LDAP_TAG_ExtendedResponse = 24
};

enum ldap_auth_mechanism {
	LDAP_AUTH_MECH_SIMPLE = 0,
	LDAP_AUTH_MECH_SASL = 3
};

#ifndef LDAP_SUCCESS
enum ldap_result_code {
	LDAP_SUCCESS = 0,
	LDAP_SASL_BIND_IN_PROGRESS = 0x0e,
	LDAP_INVALID_CREDENTIALS = 0x31,
	LDAP_OTHER = 0x50
};
#endif /* LDAP_SUCCESS */

struct ldap_Result {
	int resultcode;
	const char *dn;
	const char *errormessage;
	const char *referral;
};

struct ldap_attribute {
	const char *name;
	int num_values;
	DATA_BLOB *values;
};

struct ldap_BindRequest {
	int version;
	const char *dn;
	enum ldap_auth_mechanism mechanism;
	union {
		const char *password;
		struct {
			const char *mechanism;
			DATA_BLOB secblob;
		} SASL;
	} creds;
};

struct ldap_BindResponse {
	struct ldap_Result response;
	union {
		DATA_BLOB secblob;
	} SASL;
};

struct ldap_UnbindRequest {
	uint8 __dummy;
};

enum ldap_scope {
	LDAP_SEARCH_SCOPE_BASE = 0,
	LDAP_SEARCH_SCOPE_SINGLE = 1,
	LDAP_SEARCH_SCOPE_SUB = 2
};

enum ldap_deref {
	LDAP_DEREFERENCE_NEVER = 0,
	LDAP_DEREFERENCE_IN_SEARCHING = 1,
	LDAP_DEREFERENCE_FINDING_BASE = 2,
	LDAP_DEREFERENCE_ALWAYS
};

struct ldap_SearchRequest {
	const char *basedn;
	enum ldap_scope scope;
	enum ldap_deref deref;
	uint32 timelimit;
	uint32 sizelimit;
	BOOL attributesonly;
	char *filter;
	int num_attributes;
	const char **attributes;
};

struct ldap_SearchResEntry {
	const char *dn;
	int num_attributes;
	struct ldap_attribute *attributes;
};

struct ldap_SearchResRef {
	int num_referrals;
	const char **referrals;
};

enum ldap_modify_type {
	LDAP_MODIFY_NONE = -1,
	LDAP_MODIFY_ADD = 0,
	LDAP_MODIFY_DELETE = 1,
	LDAP_MODIFY_REPLACE = 2
};

struct ldap_mod {
	enum ldap_modify_type type;
	struct ldap_attribute attrib;
};

struct ldap_ModifyRequest {
	const char *dn;
	int num_mods;
	struct ldap_mod *mods;
};

struct ldap_AddRequest {
	const char *dn;
	int num_attributes;
	struct ldap_attribute *attributes;
};

struct ldap_DelRequest {
	const char *dn;
};

struct ldap_ModifyDNRequest {
	const char *dn;
	const char *newrdn;
	BOOL deleteolddn;
	const char *newsuperior;
};

struct ldap_CompareRequest {
	const char *dn;
	const char *attribute;
	const char *value;
};

struct ldap_AbandonRequest {
	uint32 messageid;
};

struct ldap_ExtendedRequest {
	const char *oid;
	DATA_BLOB value;
};

struct ldap_ExtendedResponse {
	struct ldap_Result response;
	const char *name;
	DATA_BLOB value;
};

union ldap_Request {
	struct ldap_BindRequest 	BindRequest;
	struct ldap_BindResponse 	BindResponse;
	struct ldap_UnbindRequest 	UnbindRequest;
	struct ldap_SearchRequest 	SearchRequest;
	struct ldap_SearchResEntry 	SearchResultEntry;
	struct ldap_Result 		SearchResultDone;
	struct ldap_SearchResRef 	SearchResultReference;
	struct ldap_ModifyRequest 	ModifyRequest;
	struct ldap_Result 		ModifyResponse;
	struct ldap_AddRequest 		AddRequest;
	struct ldap_Result 		AddResponse;
	struct ldap_DelRequest 		DelRequest;
	struct ldap_Result 		DelResponse;
	struct ldap_ModifyDNRequest 	ModifyDNRequest;
	struct ldap_Result 		ModifyDNResponse;
	struct ldap_CompareRequest 	CompareRequest;
	struct ldap_Result 		CompareResponse;
	struct ldap_AbandonRequest 	AbandonRequest;
	struct ldap_ExtendedRequest 	ExtendedRequest;
	struct ldap_ExtendedResponse 	ExtendedResponse;
};

struct ldap_Control {
	const char *oid;
	BOOL        critical;
	DATA_BLOB   value;
};

struct ldap_message {
	TALLOC_CTX	       *mem_ctx;
	uint32                  messageid;
	uint8                   type;
	union  ldap_Request     r;
	int			num_controls;
	struct ldap_Control    *controls;
};

struct ldap_queue_entry {
	struct ldap_queue_entry *next, *prev;
	int msgid;
	struct ldap_message *msg;
};

struct ldap_connection {
	TALLOC_CTX *mem_ctx;
	int sock;
	int next_msgid;
	char *host;
	uint16 port;
	BOOL ldaps;

	const char *auth_dn;
	const char *simple_pw;

	/* Current outstanding search entry */
	int searchid;

	/* List for incoming search entries */
	struct ldap_queue_entry *search_entries;

	/* Outstanding LDAP requests that have not yet been replied to */
	struct ldap_queue_entry *outstanding;
};

#endif

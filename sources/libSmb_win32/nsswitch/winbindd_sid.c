/* 
   Unix SMB/CIFS implementation.

   Winbind daemon - sid related functions

   Copyright (C) Tim Potter 2000
   
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
#include "winbindd.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

/* Convert a string  */

static void lookupsid_recv(void *private_data, BOOL success,
			   const char *dom_name, const char *name,
			   enum SID_NAME_USE type);

void winbindd_lookupsid(struct winbindd_cli_state *state)
{
	DOM_SID sid;

	/* Ensure null termination */
	state->request.data.sid[sizeof(state->request.data.sid)-1]='\0';

	DEBUG(3, ("[%5lu]: lookupsid %s\n", (unsigned long)state->pid, 
		  state->request.data.sid));

	if (!string_to_sid(&sid, state->request.data.sid)) {
		DEBUG(5, ("%s not a SID\n", state->request.data.sid));
		request_error(state);
		return;
	}

	winbindd_lookupsid_async(state->mem_ctx, &sid, lookupsid_recv, state);
}

static void lookupsid_recv(void *private_data, BOOL success,
			   const char *dom_name, const char *name,
			   enum SID_NAME_USE type)
{
	struct winbindd_cli_state *state =
		talloc_get_type_abort(private_data, struct winbindd_cli_state);

	if (!success) {
		DEBUG(5, ("lookupsid returned an error\n"));
		request_error(state);
		return;
	}

	fstrcpy(state->response.data.name.dom_name, dom_name);
	fstrcpy(state->response.data.name.name, name);
	state->response.data.name.type = type;
	request_ok(state);
}

/**
 * Look up the SID for a qualified name.  
 **/

static void lookupname_recv(void *private_data, BOOL success,
			    const DOM_SID *sid, enum SID_NAME_USE type);

void winbindd_lookupname(struct winbindd_cli_state *state)
{
	char *name_domain, *name_user;
	char *p;

	/* Ensure null termination */
	state->request.data.name.dom_name[sizeof(state->request.data.name.dom_name)-1]='\0';

	/* Ensure null termination */
	state->request.data.name.name[sizeof(state->request.data.name.name)-1]='\0';

	/* cope with the name being a fully qualified name */
	p = strstr(state->request.data.name.name, lp_winbind_separator());
	if (p) {
		*p = 0;
		name_domain = state->request.data.name.name;
		name_user = p+1;
	} else {
		name_domain = state->request.data.name.dom_name;
		name_user = state->request.data.name.name;
	}

	DEBUG(3, ("[%5lu]: lookupname %s%s%s\n", (unsigned long)state->pid,
		  name_domain, lp_winbind_separator(), name_user));

	winbindd_lookupname_async(state->mem_ctx, name_domain, name_user,
				  lookupname_recv, state);
}

static void lookupname_recv(void *private_data, BOOL success,
			    const DOM_SID *sid, enum SID_NAME_USE type)
{
	struct winbindd_cli_state *state =
		talloc_get_type_abort(private_data, struct winbindd_cli_state);

	if (!success) {
		DEBUG(5, ("lookupname returned an error\n"));
		request_error(state);
		return;
	}

	sid_to_string(state->response.data.sid.sid, sid);
	state->response.data.sid.type = type;
	request_ok(state);
	return;
}

static struct winbindd_child static_idmap_child;

void init_idmap_child(void)
{
	setup_domain_child(NULL, &static_idmap_child, "idmap");
}

struct winbindd_child *idmap_child(void)
{
	return &static_idmap_child;
}

/* Convert a sid to a uid.  We assume we only have one rid attached to the
   sid. */

static void sid2uid_recv(void *private_data, BOOL success, uid_t uid);

void winbindd_sid_to_uid(struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS result;

	/* Ensure null termination */
	state->request.data.sid[sizeof(state->request.data.sid)-1]='\0';

	DEBUG(3, ("[%5lu]: sid to uid %s\n", (unsigned long)state->pid,
		  state->request.data.sid));

	if (idmap_proxyonly()) {
		DEBUG(8, ("IDMAP proxy only\n"));
		request_error(state);
		return;
	}

	if (!string_to_sid(&sid, state->request.data.sid)) {
		DEBUG(1, ("Could not get convert sid %s from string\n",
			  state->request.data.sid));
		request_error(state);
		return;
	}

	/* Query only the local tdb, everything else might possibly block */

	result = idmap_sid_to_uid(&sid, &(state->response.data.uid),
				  ID_QUERY_ONLY|ID_CACHE_ONLY);

	if (NT_STATUS_IS_OK(result)) {
		request_ok(state);
		return;
	}

	winbindd_sid2uid_async(state->mem_ctx, &sid, sid2uid_recv, state);
}

static void sid2uid_recv(void *private_data, BOOL success, uid_t uid)
{
	struct winbindd_cli_state *state =
		talloc_get_type_abort(private_data, struct winbindd_cli_state);

	if (!success) {
		DEBUG(5, ("Could not convert sid %s\n",
			  state->request.data.sid));
		request_error(state);
		return;
	}

	state->response.data.uid = uid;
	request_ok(state);
}

/* Convert a sid to a gid.  We assume we only have one rid attached to the
   sid.*/

static void sid2gid_recv(void *private_data, BOOL success, gid_t gid);

void winbindd_sid_to_gid(struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS result;

	/* Ensure null termination */
	state->request.data.sid[sizeof(state->request.data.sid)-1]='\0';

	DEBUG(3, ("[%5lu]: sid to gid %s\n", (unsigned long)state->pid,
		  state->request.data.sid));

	if (idmap_proxyonly()) {
		DEBUG(8, ("IDMAP proxy only\n"));
		request_error(state);
		return;
	}

	if (!string_to_sid(&sid, state->request.data.sid)) {
		DEBUG(1, ("Could not get convert sid %s from string\n",
			  state->request.data.sid));
		request_error(state);
		return;
	}

	/* Query only the local tdb, everything else might possibly block */

	result = idmap_sid_to_gid(&sid, &(state->response.data.gid),
				  ID_QUERY_ONLY|ID_CACHE_ONLY);

	if (NT_STATUS_IS_OK(result)) {
		request_ok(state);
		return;
	}

	winbindd_sid2gid_async(state->mem_ctx, &sid, sid2gid_recv, state);
}

static void sid2gid_recv(void *private_data, BOOL success, gid_t gid)
{
	struct winbindd_cli_state *state =
		talloc_get_type_abort(private_data, struct winbindd_cli_state);

	if (!success) {
		DEBUG(5, ("Could not convert sid %s\n",
			  state->request.data.sid));
		request_error(state);
		return;
	}

	state->response.data.gid = gid;
	request_ok(state);
}

/* Convert a uid to a sid */

struct uid2sid_state {
	struct winbindd_cli_state *cli_state;
	uid_t uid;
	fstring name;
	DOM_SID sid;
	enum SID_NAME_USE type;
};

static void uid2sid_uid2name_recv(void *private_data, BOOL success,
				  const char *username);
static void uid2sid_lookupname_recv(void *private_data, BOOL success,
				    const DOM_SID *sid,
				    enum SID_NAME_USE type);
static void uid2sid_idmap_set_mapping_recv(void *private_data, BOOL success);

static void uid2sid_recv(void *private_data, BOOL success, const char *sid);

void winbindd_uid_to_sid(struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS status;

	DEBUG(3, ("[%5lu]: uid to sid %lu\n", (unsigned long)state->pid, 
		  (unsigned long)state->request.data.uid));

	if (idmap_proxyonly()) {
		DEBUG(8, ("IDMAP proxy only\n"));
		request_error(state);
		return;
	}

	status = idmap_uid_to_sid(&sid, state->request.data.uid,
				  ID_QUERY_ONLY | ID_CACHE_ONLY);

	if (NT_STATUS_IS_OK(status)) {
		sid_to_string(state->response.data.sid.sid, &sid);
		state->response.data.sid.type = SID_NAME_USER;
		request_ok(state);
		return;
	}

	winbindd_uid2sid_async(state->mem_ctx, state->request.data.uid, uid2sid_recv, state);
}

static void uid2sid_recv(void *private_data, BOOL success, const char *sid)
{
	struct winbindd_cli_state *state = private_data;
	struct uid2sid_state *uid2sid_state;

	if (success) {
		DEBUG(10,("uid2sid: uid %lu has sid %s\n",
			  (unsigned long)(state->request.data.uid), sid));
		fstrcpy(state->response.data.sid.sid, sid);
		state->response.data.sid.type = SID_NAME_USER;
		request_ok(state);
		return;
	}

	/* preexisitng mapping not found go on */

	if (is_in_uid_range(state->request.data.uid)) {
		/* This is winbind's, so we should better have succeeded
		 * above. */
		request_error(state);
		return;
	}

	/* The only chance that this is correct is that winbind trusted
	 * domains only = yes, and the user exists in nss and the domain. */

	if (!lp_winbind_trusted_domains_only()) {
		request_error(state);
		return;
	}

	uid2sid_state = TALLOC_ZERO_P(state->mem_ctx, struct uid2sid_state);
	if (uid2sid_state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		request_error(state);
		return;
	}

	uid2sid_state->cli_state = state;
	uid2sid_state->uid = state->request.data.uid;

	winbindd_uid2name_async(state->mem_ctx, state->request.data.uid,
				uid2sid_uid2name_recv, uid2sid_state);
}

static void uid2sid_uid2name_recv(void *private_data, BOOL success,
				  const char *username)
{
	struct uid2sid_state *state =
		talloc_get_type_abort(private_data, struct uid2sid_state);

	DEBUG(10, ("uid2sid: uid %lu has name %s\n",
		   (unsigned long)state->uid, username));

	fstrcpy(state->name, username);

	if (!success) {
		request_error(state->cli_state);
		return;
	}

	winbindd_lookupname_async(state->cli_state->mem_ctx,
				  find_our_domain()->name, username,
				  uid2sid_lookupname_recv, state);
}

static void uid2sid_lookupname_recv(void *private_data, BOOL success,
				    const DOM_SID *sid, enum SID_NAME_USE type)
{
	struct uid2sid_state *state =
		talloc_get_type_abort(private_data, struct uid2sid_state);
	unid_t id;

	if ((!success) || (type != SID_NAME_USER)) {
		request_error(state->cli_state);
		return;
	}

	state->sid = *sid;
	state->type = type;

	id.uid = state->uid;
	idmap_set_mapping_async(state->cli_state->mem_ctx, sid, id, ID_USERID,
				uid2sid_idmap_set_mapping_recv, state );
}

static void uid2sid_idmap_set_mapping_recv(void *private_data, BOOL success)
{
	struct uid2sid_state *state =
		talloc_get_type_abort(private_data, struct uid2sid_state);

	/* don't fail if we can't store it */

	sid_to_string(state->cli_state->response.data.sid.sid, &state->sid);
	state->cli_state->response.data.sid.type = state->type;
	request_ok(state->cli_state);
}

/* Convert a gid to a sid */

struct gid2sid_state {
	struct winbindd_cli_state *cli_state;
	gid_t gid;
	fstring name;
	DOM_SID sid;
	enum SID_NAME_USE type;
};

static void gid2sid_gid2name_recv(void *private_data, BOOL success,
				  const char *groupname);
static void gid2sid_lookupname_recv(void *private_data, BOOL success,
				    const DOM_SID *sid,
				    enum SID_NAME_USE type);
static void gid2sid_idmap_set_mapping_recv(void *private_data, BOOL success);

static void gid2sid_recv(void *private_data, BOOL success, const char *sid);

void winbindd_gid_to_sid(struct winbindd_cli_state *state)
{
	DOM_SID sid;
	NTSTATUS status;

	DEBUG(3, ("[%5lu]: gid to sid %lu\n", (unsigned long)state->pid, 
		  (unsigned long)state->request.data.gid));

	if (idmap_proxyonly()) {
		DEBUG(8, ("IDMAP proxy only\n"));
		request_error(state);
		return;
	}

	status = idmap_gid_to_sid(&sid, state->request.data.gid,
				  ID_QUERY_ONLY | ID_CACHE_ONLY);

	if (NT_STATUS_IS_OK(status)) {
		sid_to_string(state->response.data.sid.sid, &sid);
		state->response.data.sid.type = SID_NAME_DOM_GRP;
		request_ok(state);
		return;
	}

	winbindd_gid2sid_async(state->mem_ctx, state->request.data.gid, gid2sid_recv, state);
}

static void gid2sid_recv(void *private_data, BOOL success, const char *sid)
{
	struct winbindd_cli_state *state = private_data;
	struct gid2sid_state *gid2sid_state;

	if (success) {
		DEBUG(10,("gid2sid: gid %lu has sid %s\n",
			  (unsigned long)(state->request.data.gid), sid));
		fstrcpy(state->response.data.sid.sid, sid);
		state->response.data.sid.type = SID_NAME_DOM_GRP;
		request_ok(state);
		return;
	}

	/* preexisitng mapping not found go on */

	if (is_in_gid_range(state->request.data.gid)) {
		/* This is winbind's, so we should better have succeeded
		 * above. */
		request_error(state);
		return;
	}

	/* The only chance that this is correct is that winbind trusted
	 * domains only = yes, and the user exists in nss and the domain. */

	if (!lp_winbind_trusted_domains_only()) {
		request_error(state);
		return;
	}

	/* The only chance that this is correct is that winbind trusted
	 * domains only = yes, and the user exists in nss and the domain. */

	gid2sid_state = TALLOC_ZERO_P(state->mem_ctx, struct gid2sid_state);
	if (gid2sid_state == NULL) {
		DEBUG(0, ("talloc failed\n"));
		request_error(state);
		return;
	}

	gid2sid_state->cli_state = state;
	gid2sid_state->gid = state->request.data.gid;

	winbindd_gid2name_async(state->mem_ctx, state->request.data.gid,
				gid2sid_gid2name_recv, gid2sid_state);
}

static void gid2sid_gid2name_recv(void *private_data, BOOL success,
				  const char *username)
{
	struct gid2sid_state *state =
		talloc_get_type_abort(private_data, struct gid2sid_state);

	DEBUG(10, ("gid2sid: gid %lu has name %s\n",
		   (unsigned long)state->gid, username));

	fstrcpy(state->name, username);

	if (!success) {
		request_error(state->cli_state);
		return;
	}

	winbindd_lookupname_async(state->cli_state->mem_ctx,
				  find_our_domain()->name, username,
				  gid2sid_lookupname_recv, state);
}

static void gid2sid_lookupname_recv(void *private_data, BOOL success,
				    const DOM_SID *sid, enum SID_NAME_USE type)
{
	struct gid2sid_state *state =
		talloc_get_type_abort(private_data, struct gid2sid_state);
	unid_t id;

	if ((!success) ||
	    ((type != SID_NAME_DOM_GRP) && (type!=SID_NAME_ALIAS))) {
		request_error(state->cli_state);
		return;
	}

	state->sid = *sid;
	state->type = type;

	id.gid = state->gid;
	idmap_set_mapping_async(state->cli_state->mem_ctx, sid, id, ID_GROUPID,
				gid2sid_idmap_set_mapping_recv, state );
}

static void gid2sid_idmap_set_mapping_recv(void *private_data, BOOL success)
{
	struct gid2sid_state *state = private_data;

	/* don't fail if we can't store it */

	sid_to_string(state->cli_state->response.data.sid.sid, &state->sid);
	state->cli_state->response.data.sid.type = state->type;
	request_ok(state->cli_state);
}

void winbindd_allocate_uid(struct winbindd_cli_state *state)
{
	if ( !state->privileged ) {
		DEBUG(2, ("winbindd_allocate_uid: non-privileged access "
			  "denied!\n"));
		request_error(state);
		return;
	}

	sendto_child(state, idmap_child());
}

enum winbindd_result winbindd_dual_allocate_uid(struct winbindd_domain *domain,
						struct winbindd_cli_state *state)
{
	union unid_t id;

	if (!NT_STATUS_IS_OK(idmap_allocate_id(&id, ID_USERID))) {
		return WINBINDD_ERROR;
	}
	state->response.data.uid = id.uid;
	return WINBINDD_OK;
}

void winbindd_allocate_gid(struct winbindd_cli_state *state)
{
	if ( !state->privileged ) {
		DEBUG(2, ("winbindd_allocate_gid: non-privileged access "
			  "denied!\n"));
		request_error(state);
		return;
	}

	sendto_child(state, idmap_child());
}

enum winbindd_result winbindd_dual_allocate_gid(struct winbindd_domain *domain,
						struct winbindd_cli_state *state)
{
	union unid_t id;

	if (!NT_STATUS_IS_OK(idmap_allocate_id(&id, ID_GROUPID))) {
		return WINBINDD_ERROR;
	}
	state->response.data.gid = id.gid;
	return WINBINDD_OK;
}


/* 
   Unix SMB/CIFS implementation.

   Python wrapper for winbind client functions.

   Copyright (C) Tim Potter      2002-2003
   
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

#include "py_winbind.h"

/* 
 * Exceptions raised by this module 
 */

PyObject *winbind_error;	/* A winbind call returned WINBINDD_ERROR */

/* Prototypes from common.h */

NSS_STATUS winbindd_request_response(int req_type, 
			    struct winbindd_request *request,
			    struct winbindd_response *response);

/*
 * Name <-> SID conversion
 */

/* Convert a name to a sid */

static PyObject *py_name_to_sid(PyObject *self, PyObject *args)

{
	struct winbindd_request request;
	struct winbindd_response response;
	PyObject *result;
	char *name, *p;
	const char *sep;

	if (!PyArg_ParseTuple(args, "s", &name))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	sep = lp_winbind_separator();

	if ((p = strchr(name, sep[0]))) {
		*p = 0;
		fstrcpy(request.data.name.dom_name, name);
		fstrcpy(request.data.name.name, p + 1);
	} else {
		fstrcpy(request.data.name.dom_name, lp_workgroup());
		fstrcpy(request.data.name.name, name);
	}

	if (winbindd_request_response(WINBINDD_LOOKUPNAME, &request, &response)  
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;
	}

	result = PyString_FromString(response.data.sid.sid);

	return result;
}

/* Convert a sid to a name */

static PyObject *py_sid_to_name(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	PyObject *result;
	char *sid, *name;

	if (!PyArg_ParseTuple(args, "s", &sid))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.sid, sid);

	if (winbindd_request_response(WINBINDD_LOOKUPSID, &request, &response)  
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;
	}

	asprintf(&name, "%s%s%s", response.data.name.dom_name,
		 lp_winbind_separator(), response.data.name.name);

	result = PyString_FromString(name);

	free(name);

	return result;
}

/*
 * Enumerate users/groups
 */

/* Enumerate domain users */

static PyObject *py_enum_domain_users(PyObject *self, PyObject *args)
{
	struct winbindd_response response;
	PyObject *result;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	ZERO_STRUCT(response);

	if (winbindd_request_response(WINBINDD_LIST_USERS, NULL, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}

	result = PyList_New(0);

	if (response.extra_data.data) {
		const char *extra_data = response.extra_data.data;
		fstring name;

		while (next_token(&extra_data, name, ",", sizeof(fstring)))
			PyList_Append(result, PyString_FromString(name));
	}

	return result;
}

/* Enumerate domain groups */

static PyObject *py_enum_domain_groups(PyObject *self, PyObject *args)
{
	struct winbindd_response response;
	PyObject *result = NULL;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	ZERO_STRUCT(response);

	if (winbindd_request_response(WINBINDD_LIST_GROUPS, NULL, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}

	result = PyList_New(0);

	if (response.extra_data.data) {
		const char *extra_data = response.extra_data.data;
		fstring name;

		while (next_token(&extra_data, name, ",", sizeof(fstring)))
			PyList_Append(result, PyString_FromString(name));
	}

	return result;
}

/*
 * Miscellaneous domain related
 */

/* Enumerate domain groups */

static PyObject *py_enum_trust_dom(PyObject *self, PyObject *args)
{
	struct winbindd_response response;
	PyObject *result = NULL;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	ZERO_STRUCT(response);

	if (winbindd_request_response(WINBINDD_LIST_TRUSTDOM, NULL, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}

	result = PyList_New(0);

	if (response.extra_data.data) {
		const char *extra_data = response.extra_data.data;
		fstring name;

		while (next_token(&extra_data, name, ",", sizeof(fstring)))
			PyList_Append(result, PyString_FromString(name));
	}

	return result;
}

/* Check machine account password */

static PyObject *py_check_secret(PyObject *self, PyObject *args)
{
	struct winbindd_response response;

	if (!PyArg_ParseTuple(args, ""))
		return NULL;

	ZERO_STRUCT(response);

	if (winbindd_request_response(WINBINDD_CHECK_MACHACC, NULL, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}

	return PyInt_FromLong(response.data.num_entries);
}

/*
 * Return a dictionary consisting of all the winbind related smb.conf
 * parameters.  This is stored in the module object.
 */

static PyObject *py_config_dict(void)
{
	PyObject *result;
	uid_t ulow, uhi;
	gid_t glow, ghi;
	
	if (!(result = PyDict_New()))
		return NULL;

	/* Various string parameters */

	PyDict_SetItemString(result, "workgroup", 
			     PyString_FromString(lp_workgroup()));

	PyDict_SetItemString(result, "separator", 
			     PyString_FromString(lp_winbind_separator()));

	PyDict_SetItemString(result, "template_homedir", 
			     PyString_FromString(lp_template_homedir()));

	PyDict_SetItemString(result, "template_shell", 
			     PyString_FromString(lp_template_shell()));

	/* idmap uid/gid range */

	if (lp_idmap_uid(&ulow, &uhi)) {
		PyDict_SetItemString(result, "uid_low", PyInt_FromLong(ulow));
		PyDict_SetItemString(result, "uid_high", PyInt_FromLong(uhi));
	}

	if (lp_idmap_gid(&glow, &ghi)) {
		PyDict_SetItemString(result, "gid_low", PyInt_FromLong(glow));
		PyDict_SetItemString(result, "gid_high", PyInt_FromLong(ghi));
	}

	return result;
}

/*
 * ID mapping
 */

/* Convert a uid to a SID */

static PyObject *py_uid_to_sid(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int id;

	if (!PyArg_ParseTuple(args, "i", &id))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.uid = id;

	if (winbindd_request_response(WINBINDD_UID_TO_SID, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}

	return PyString_FromString(response.data.sid.sid);
}

/* Convert a gid to a SID */

static PyObject *py_gid_to_sid(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	int id;

	if (!PyArg_ParseTuple(args, "i", &id))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.gid = id;

	if (winbindd_request_response(WINBINDD_GID_TO_SID, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}

	return PyString_FromString(response.data.sid.sid);
}

/* Convert a sid to a uid */

static PyObject *py_sid_to_uid(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	char *sid;

	if (!PyArg_ParseTuple(args, "s", &sid))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.sid, sid);

	if (winbindd_request_response(WINBINDD_SID_TO_UID, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}

	return PyInt_FromLong(response.data.uid);
}

/* Convert a sid to a gid */

static PyObject *py_sid_to_gid(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	char *sid;

	if (!PyArg_ParseTuple(args, "s", &sid))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.sid, sid);

	if (winbindd_request_response(WINBINDD_SID_TO_GID, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}
	
	return PyInt_FromLong(response.data.gid);
}

/*
 * PAM authentication functions
 */

/* Plaintext authentication */

static PyObject *py_auth_plaintext(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	char *username, *password;

	if (!PyArg_ParseTuple(args, "ss", &username, &password))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.auth.user, username);
	fstrcpy(request.data.auth.pass, password);

	if (winbindd_request_response(WINBINDD_PAM_AUTH, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}
	
	return PyInt_FromLong(response.data.auth.nt_status);
}

/* Challenge/response authentication */

static PyObject *py_auth_crap(PyObject *self, PyObject *args, PyObject *kw)
{
	static char *kwlist[] = 
		{"username", "password", "use_lm_hash", "use_nt_hash", NULL };
	struct winbindd_request request;
	struct winbindd_response response;
	char *username, *password;
	int use_lm_hash = 1, use_nt_hash = 1;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "ss|ii", kwlist, &username, &password, 
		    &use_lm_hash, &use_nt_hash))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	if (push_utf8_fstring(request.data.auth_crap.user, username) == -1) {
		PyErr_SetString(winbind_error, "unable to create utf8 string");
		return NULL;
	}

	generate_random_buffer(request.data.auth_crap.chal, 8);
        
	if (use_lm_hash) {
		SMBencrypt((uchar *)password, request.data.auth_crap.chal, 
			   (uchar *)request.data.auth_crap.lm_resp);
		request.data.auth_crap.lm_resp_len = 24;
	}

	if (use_nt_hash) {
		SMBNTencrypt((uchar *)password, request.data.auth_crap.chal,
			     (uchar *)request.data.auth_crap.nt_resp);
		request.data.auth_crap.nt_resp_len = 24;
	}

	if (winbindd_request_response(WINBINDD_PAM_AUTH_CRAP, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}
	
	return PyInt_FromLong(response.data.auth.nt_status);
}

#if 0				/* Include when auth_smbd merged to HEAD */

/* Challenge/response authentication, with secret */

static PyObject *py_auth_smbd(PyObject *self, PyObject *args, PyObject *kw)
{
	static char *kwlist[] = 
		{"username", "password", "use_lm_hash", "use_nt_hash", NULL };
	struct winbindd_request request;
	struct winbindd_response response;
	char *username, *password;
	int use_lm_hash = 1, use_nt_hash = 1;

	if (!PyArg_ParseTupleAndKeywords(
		    args, kw, "ss|ii", kwlist, &username, &password, 
		    &use_lm_hash, &use_nt_hash))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	if (push_utf8_fstring(request.data.auth_crap.user, username) == -1) {
		PyErr_SetString("unable to create utf8 string");
		return NULL;
	}

	generate_random_buffer(request.data.smbd_auth_crap.chal, 8);
        
	if (use_lm_hash) {
		SMBencrypt((uchar *)password, 
			   request.data.smbd_auth_crap.chal, 
			   (uchar *)request.data.smbd_auth_crap.lm_resp);
		request.data.smbd_auth_crap.lm_resp_len = 24;
	}

	if (use_nt_hash) {
		SMBNTencrypt((uchar *)password, 
			     request.data.smbd_auth_crap.chal,
			     (uchar *)request.data.smbd_auth_crap.nt_resp);
		request.data.smbd_auth_crap.nt_resp_len = 24;
	}

	if (!secrets_fetch_trust_account_password(
		    lp_workgroup(), request.data.smbd_auth_crap.proof, NULL)) {
		PyErr_SetString(
			winbind_error, "unable to fetch domain secret");
		return NULL;
	}



	if (winbindd_request_response(WINBINDD_SMBD_AUTH_CRAP, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}
	
	return PyInt_FromLong(response.data.auth.nt_status);
}

#endif /* 0 */

/* Get user info from name */

static PyObject *py_getpwnam(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	char *username;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "s", &username))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	fstrcpy(request.data.username, username);

	if (winbindd_request_response(WINBINDD_GETPWNAM, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}
	
	if (!py_from_winbind_passwd(&result, &response)) {
		result = Py_None;
		Py_INCREF(result);
	}

	return result;
}

/* Get user info from uid */

static PyObject *py_getpwuid(PyObject *self, PyObject *args)
{
	struct winbindd_request request;
	struct winbindd_response response;
	uid_t uid;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "i", &uid))
		return NULL;

	ZERO_STRUCT(request);
	ZERO_STRUCT(response);

	request.data.uid = uid;

	if (winbindd_request_response(WINBINDD_GETPWUID, &request, &response) 
	    != NSS_STATUS_SUCCESS) {
		PyErr_SetString(winbind_error, "lookup failed");
		return NULL;		
	}
	
	if (!py_from_winbind_passwd(&result, &response)) {
		result = Py_None;
		Py_INCREF(result);
	}

	return result;
}

/*
 * Method dispatch table
 */

static PyMethodDef winbind_methods[] = {

	{ "getpwnam", (PyCFunction)py_getpwnam, METH_VARARGS, "getpwnam(3)" },
	{ "getpwuid", (PyCFunction)py_getpwuid, METH_VARARGS, "getpwuid(3)" },

	/* Name <-> SID conversion */

	{ "name_to_sid", (PyCFunction)py_name_to_sid, METH_VARARGS,
	  "name_to_sid(s) -> string\n"
"\n"
"Return the SID for a name.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.name_to_sid('FOO/Administrator')\n"
"'S-1-5-21-406022937-1377575209-526660263-500' " },

	{ "sid_to_name", (PyCFunction)py_sid_to_name, METH_VARARGS,
	  "sid_to_name(s) -> string\n"
"\n"
"Return the name for a SID.\n"
"\n"
"Example:\n"
"\n"
">>> import winbind\n"
">>> winbind.sid_to_name('S-1-5-21-406022937-1377575209-526660263-500')\n"
"'FOO/Administrator' " },

	/* Enumerate users/groups */

	{ "enum_domain_users", (PyCFunction)py_enum_domain_users, METH_VARARGS,
	  "enum_domain_users() -> list of strings\n"
"\n"
"Return a list of domain users.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.enum_domain_users()\n"
"['FOO/Administrator', 'FOO/anna', 'FOO/Anne Elk', 'FOO/build', \n"
"'FOO/foo', 'FOO/foo2', 'FOO/foo3', 'FOO/Guest', 'FOO/user1', \n"
"'FOO/whoops-ptang'] " },

	{ "enum_domain_groups", (PyCFunction)py_enum_domain_groups, 
	  METH_VARARGS,
	  "enum_domain_groups() -> list of strings\n"
"\n"
"Return a list of domain groups.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.enum_domain_groups()\n"
"['FOO/cows', 'FOO/Domain Admins', 'FOO/Domain Guests', \n"
"'FOO/Domain Users'] " },

	/* ID mapping */

	{ "uid_to_sid", (PyCFunction)py_uid_to_sid, METH_VARARGS,
	  "uid_to_sid(int) -> string\n"
"\n"
"Return the SID for a UNIX uid.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.uid_to_sid(10000)   \n"
"'S-1-5-21-406022937-1377575209-526660263-500' " },

	{ "gid_to_sid", (PyCFunction)py_gid_to_sid, METH_VARARGS,
	  "gid_to_sid(int) -> string\n"
"\n"
"Return the UNIX gid for a SID.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.gid_to_sid(10001)\n"
"'S-1-5-21-406022937-1377575209-526660263-512' " },

	{ "sid_to_uid", (PyCFunction)py_sid_to_uid, METH_VARARGS,
	  "sid_to_uid(string) -> int\n"
"\n"
"Return the UNIX uid for a SID.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.sid_to_uid('S-1-5-21-406022937-1377575209-526660263-500')\n"
"10000 " },

	{ "sid_to_gid", (PyCFunction)py_sid_to_gid, METH_VARARGS,
	  "sid_to_gid(string) -> int\n"
"\n"
"Return the UNIX gid corresponding to a SID.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.sid_to_gid('S-1-5-21-406022937-1377575209-526660263-512')\n"
"10001 " },

	/* Miscellaneous */

	{ "check_secret", (PyCFunction)py_check_secret, METH_VARARGS,
	  "check_secret() -> int\n"
"\n"
"Check the machine trust account password.  The NT status is returned\n"
"with zero indicating success. " },

	{ "enum_trust_dom", (PyCFunction)py_enum_trust_dom, METH_VARARGS,
	  "enum_trust_dom() -> list of strings\n"
"\n"
"Return a list of trusted domains.  The domain the server is a member \n"
"of is not included.\n"
"\n"
"Example:\n"
"\n"
">>> winbind.enum_trust_dom()\n"
"['NPSD-TEST2', 'SP2NDOM'] " },

	/* PAM authorisation functions */

	{ "auth_plaintext", (PyCFunction)py_auth_plaintext, METH_VARARGS,
	  "auth_plaintext(s, s) -> int\n"
"\n"
"Authenticate a username and password using plaintext authentication.\n"
"The NT status code is returned with zero indicating success." },

	{ "auth_crap", (PyCFunction)py_auth_crap, METH_VARARGS | METH_KEYWORDS,
	  "auth_crap(s, s) -> int\n"
"\n"
"Authenticate a username and password using the challenge/response\n"
"protocol.  The NT status code is returned with zero indicating\n"
"success." },

#if 0				/* Include when smbd_auth merged to HEAD */

	{ "auth_smbd", (PyCFunction)py_auth_crap, METH_VARARGS,
	  "auth_smbd(s, s) -> int\n"
"\n"
"Authenticate a username and password using the challenge/response\n"
"protocol but using the domain secret to prove we are root.  The NT \n"
"status code is returned with zero indicating success." },

#endif

	{ NULL }
};

static struct const_vals {
	char *name;
	uint32 value;
	char *docstring;
} module_const_vals[] = {

	/* Well known RIDs */
	
	{ "DOMAIN_USER_RID_ADMIN", DOMAIN_USER_RID_ADMIN, 
	  "Well-known RID for Administrator user" },

	{ "DOMAIN_USER_RID_GUEST", DOMAIN_USER_RID_GUEST,
	  "Well-known RID for Guest user" },

	{ "DOMAIN_GROUP_RID_ADMINS", DOMAIN_GROUP_RID_ADMINS,
	  "Well-known RID for Domain Admins group" },

	{ "DOMAIN_GROUP_RID_USERS", DOMAIN_GROUP_RID_USERS,
	  "Well-known RID for Domain Users group" },

	{ "DOMAIN_GROUP_RID_GUESTS", DOMAIN_GROUP_RID_GUESTS,
	  "Well-known RID for Domain Guests group" }, 
	
	{ NULL }
};

static void const_init(PyObject *dict)
{
	struct const_vals *tmp;
	PyObject *obj;

	for (tmp = module_const_vals; tmp->name; tmp++) {
		obj = PyInt_FromLong(tmp->value);
		PyDict_SetItemString(dict, tmp->name, obj);
		Py_DECREF(obj);
	}
}

/*
 * Module initialisation 
 */

static char winbind_module__doc__[] =
"A python extension to winbind client functions.";

void initwinbind(void)
{
	PyObject *module, *dict;

	/* Initialise module */

        module = Py_InitModule3("winbind", winbind_methods,
				winbind_module__doc__);

	dict = PyModule_GetDict(module);

	winbind_error = PyErr_NewException("winbind.error", NULL, NULL);
	PyDict_SetItemString(dict, "error", winbind_error);

	/* Do samba initialisation */

	py_samba_init();

	/* Initialise constants */

	const_init(dict);

	/* Insert configuration dictionary */

	PyDict_SetItemString(dict, "config", py_config_dict());
}

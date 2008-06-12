/* 
   Unix SMB/CIFS implementation.
   module loading system

   Copyright (C) Jelmer Vernooij 2002-2003
   Copyright (C) Stefan (metze) Metzmacher 2003
   
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

#ifdef HAVE_DLOPEN

/* Load a dynamic module.  Only log a level 0 error if we are not checking 
   for the existence of a module (probling). */

static NTSTATUS do_smb_load_module(const char *module_name, BOOL is_probe)
{
	void *handle;
	init_module_function *init;
	NTSTATUS status;
	const char *error;

	/* Always try to use LAZY symbol resolving; if the plugin has 
	 * backwards compatibility, there might be symbols in the 
	 * plugin referencing to old (removed) functions
	 */
	handle = sys_dlopen(module_name, RTLD_LAZY);

	/* This call should reset any possible non-fatal errors that 
	   occured since last call to dl* functions */
	error = sys_dlerror();

	if(!handle) {
		int level = is_probe ? 3 : 0;
		DEBUG(level, ("Error loading module '%s': %s\n", module_name, error ? error : ""));
		return NT_STATUS_UNSUCCESSFUL;
	}

	init = (init_module_function *)sys_dlsym(handle, "init_module");

	/* we must check sys_dlerror() to determine if it worked, because
           sys_dlsym() can validly return NULL */
	error = sys_dlerror();
	if (error) {
		DEBUG(0, ("Error trying to resolve symbol 'init_module' in %s: %s\n", 
			  module_name, error));
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(2, ("Module '%s' loaded\n", module_name));

	status = init();
	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0, ("Module '%s' initialization failed: %s\n",
			    module_name, get_friendly_nt_error_msg(status)));
	}

	return status;
}

NTSTATUS smb_load_module(const char *module_name)
{
	return do_smb_load_module(module_name, False);
}

/* Load all modules in list and return number of 
 * modules that has been successfully loaded */
int smb_load_modules(const char **modules)
{
	int i;
	int success = 0;

	for(i = 0; modules[i]; i++){
		if(NT_STATUS_IS_OK(smb_load_module(modules[i]))) {
			success++;
		}
	}

	DEBUG(2, ("%d modules successfully loaded\n", success));

	return success;
}

NTSTATUS smb_probe_module(const char *subsystem, const char *module)
{
	pstring full_path;
	
	/* Check for absolute path */

	/* if we make any 'samba multibyte string' 
	   calls here, we break 
	   for loading string modules */

	DEBUG(5, ("Probing module '%s'\n", module));

	if (module[0] == '/')
		return do_smb_load_module(module, True);
	
	pstrcpy(full_path, lib_path(subsystem));
	pstrcat(full_path, "/");
	pstrcat(full_path, module);
	pstrcat(full_path, ".");
	pstrcat(full_path, shlib_ext());

	DEBUG(5, ("Probing module '%s': Trying to load from %s\n", module, full_path));
	
	return do_smb_load_module(full_path, True);
}

#else /* HAVE_DLOPEN */

NTSTATUS smb_load_module(const char *module_name)
{
	DEBUG(0,("This samba executable has not been built with plugin support\n"));
	return NT_STATUS_NOT_SUPPORTED;
}

int smb_load_modules(const char **modules)
{
	DEBUG(0,("This samba executable has not been built with plugin support\n"));
	return -1;
}

NTSTATUS smb_probe_module(const char *subsystem, const char *module)
{
	DEBUG(0,("This samba executable has not been built with plugin support, not probing\n")); 
	return NT_STATUS_NOT_SUPPORTED;
}

#endif /* HAVE_DLOPEN */

void init_modules(void)
{
	/* FIXME: This can cause undefined symbol errors :
	 *  smb_register_vfs() isn't available in nmbd, for example */
	if(lp_preload_modules()) 
		smb_load_modules(lp_preload_modules());
}


/***************************************************************************
 * This Function registers a idle event
 *
 * the registered funtions are run periodically
 * and maybe shutdown idle connections (e.g. to an LDAP server)
 ***************************************************************************/
static smb_event_id_t smb_idle_event_id = 1;

struct smb_idle_list_ent {
	struct smb_idle_list_ent *prev,*next;
	smb_event_id_t id;
	smb_idle_event_fn *fn;
	void *data;
	time_t interval;
	time_t lastrun;
};

static struct smb_idle_list_ent *smb_idle_event_list = NULL;

smb_event_id_t smb_register_idle_event(smb_idle_event_fn *fn, void *data, time_t interval)
{
	struct smb_idle_list_ent *event;

	if (!fn) {	
		return SMB_EVENT_ID_INVALID;
	}

	event = SMB_MALLOC_P(struct smb_idle_list_ent);
	if (!event) {
		DEBUG(0,("malloc() failed!\n"));
		return SMB_EVENT_ID_INVALID;
	}
	event->fn = fn;
	event->data = data;
	event->interval = interval;
	event->lastrun = 0;
	event->id = smb_idle_event_id++;

	DLIST_ADD(smb_idle_event_list,event);

	return event->id;
}

BOOL smb_unregister_idle_event(smb_event_id_t id)
{
	struct smb_idle_list_ent *event = smb_idle_event_list;
	
	while(event) {
		if (event->id == id) {
			DLIST_REMOVE(smb_idle_event_list,event);
			SAFE_FREE(event);
			return True;
		}
		event = event->next;
	}
	
	return False;
}

void smb_run_idle_events(time_t now)
{
	struct smb_idle_list_ent *event = smb_idle_event_list;

	while (event) {
		struct smb_idle_list_ent *next = event->next;
		time_t interval;

		if (event->interval <= 0) {
			interval = SMB_IDLE_EVENT_DEFAULT_INTERVAL;
		} else if (event->interval >= SMB_IDLE_EVENT_MIN_INTERVAL) {
			interval = event->interval;
		} else {
			interval = SMB_IDLE_EVENT_MIN_INTERVAL;
		}
		if (now >(event->lastrun+interval)) {
			event->lastrun = now;
			event->fn(&event->data,&event->interval,now);
		}
		event = next;
	}

	return;
}

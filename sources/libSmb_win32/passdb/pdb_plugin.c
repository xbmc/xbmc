/* 
   Unix SMB/CIFS implementation.
   Loadable passdb module interface.
   Copyright (C) Jelmer Vernooij 2002
   Copyright (C) Andrew Bartlett 2002
      
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
#define DBGC_CLASS DBGC_PASSDB

NTSTATUS pdb_init_plugin(struct pdb_methods **pdb_method, const char *location)
{
	void * dl_handle;
	char *plugin_location, *plugin_name, *p;
	pdb_init_function plugin_init;
	int (*plugin_version)(void);

	if (location == NULL) {
		DEBUG(0, ("The plugin module needs an argument!\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}

	plugin_name = smb_xstrdup(location);
	p = strchr(plugin_name, ':');
	if (p) {
		*p = 0;
		plugin_location = p+1;
		trim_char(plugin_location, ' ', ' ');
	} else {
		plugin_location = NULL;
	}
	trim_char(plugin_name, ' ', ' ');

	DEBUG(5, ("Trying to load sam plugin %s\n", plugin_name));
	dl_handle = sys_dlopen(plugin_name, RTLD_NOW );
	if (!dl_handle) {
		DEBUG(0, ("Failed to load sam plugin %s using sys_dlopen (%s)\n", plugin_name, sys_dlerror()));
		return NT_STATUS_UNSUCCESSFUL;
	}
    
	plugin_version = sys_dlsym(dl_handle, "pdb_version");
	if (!plugin_version) {
		sys_dlclose(dl_handle);
		DEBUG(0, ("Failed to find function 'pdb_version' using sys_dlsym in sam plugin %s (%s)\n", plugin_name, sys_dlerror()));	    
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (plugin_version() != PASSDB_INTERFACE_VERSION) {
		sys_dlclose(dl_handle);
		DEBUG(0, ("Wrong PASSDB_INTERFACE_VERSION! sam plugin has version %d and version %d is needed! Please update!\n",
			    plugin_version(),PASSDB_INTERFACE_VERSION));
		return NT_STATUS_UNSUCCESSFUL;
	}
				    	
	plugin_init = sys_dlsym(dl_handle, "pdb_init");
	if (!plugin_init) {
		sys_dlclose(dl_handle);
		DEBUG(0, ("Failed to find function 'pdb_init' using sys_dlsym in sam plugin %s (%s)\n", plugin_name, sys_dlerror()));	    
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(5, ("Starting sam plugin %s with location %s\n", plugin_name, plugin_location));
	return plugin_init(pdb_method, plugin_location);
}

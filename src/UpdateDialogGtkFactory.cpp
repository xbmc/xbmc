#include "UpdateDialogGtkFactory.h"

#include "Log.h"
#include "UpdateDialog.h"
#include "StringUtils.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

class UpdateDialogGtk;

// GTK updater UI library embedded into
// the updater binary
extern unsigned char libupdatergtk_so[];
extern unsigned int libupdatergtk_so_len;

// pointers to helper functions in the GTK updater UI library
UpdateDialogGtk* (*update_dialog_gtk_new)() = 0;

#define BIND_FUNCTION(library,function) \
	function = reinterpret_cast<typeof(function)>(dlsym(library,#function));

bool extractFileFromBinary(const char* path, const void* buffer, size_t length)
{
	int fd = open(path,O_CREAT | O_WRONLY | O_TRUNC,0755);
	size_t count = write(fd,buffer,length);
	if (fd < 0 || count < length)
	{
		if (fd >= 0)
		{
			close(fd);
		}
		return false;
	}
	close(fd);
	return true;
}

UpdateDialog* UpdateDialogGtkFactory::createDialog()
{
	const char* libPath = "/tmp/libupdatergtk.so";

	if (!extractFileFromBinary(libPath,libupdatergtk_so,libupdatergtk_so_len))
	{
		LOG(Warn,"Failed to load the GTK UI library - " + std::string(strerror(errno)));
		return 0;
	}

	void* gtkLib = dlopen(libPath,RTLD_LAZY);
	if (!gtkLib)
	{
		LOG(Warn,"Failed to load the GTK UI - " + std::string(dlerror()));
		return 0;
	}

	BIND_FUNCTION(gtkLib,update_dialog_gtk_new);
	return reinterpret_cast<UpdateDialog*>(update_dialog_gtk_new());
}


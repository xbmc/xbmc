#include "UpdateDialogGtkWrapper.h"

#include "Log.h"
#include "StringUtils.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// GTK updater UI library embedded into
// the updater binary
extern unsigned char libupdatergtk_so[];
extern unsigned int libupdatergtk_so_len;

// pointers to helper functions in the GTK updater UI library
UpdateDialogGtk* (*update_dialog_gtk_new)(int,char**) = 0;
void (*update_dialog_gtk_exec)(UpdateDialogGtk* dialog) = 0;
void (*update_dialog_gtk_handle_error)(UpdateDialogGtk* dialog, const std::string& errorMessage) = 0;
void (*update_dialog_gtk_handle_progress)(UpdateDialogGtk* dialog, int percentage) = 0;
void (*update_dialog_gtk_handle_finished)(UpdateDialogGtk* dialog) = 0;

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

UpdateDialogGtkWrapper::UpdateDialogGtkWrapper()
: m_dialog(0)
{
}

bool UpdateDialogGtkWrapper::init(int argc, char** argv)
{
	const char* libPath = "/tmp/libupdatergtk.so";

	if (!extractFileFromBinary(libPath,libupdatergtk_so,libupdatergtk_so_len))
	{
		LOG(Warn,"Failed to load the GTK UI library - " + std::string(strerror(errno)));
	}

	void* gtkLib = dlopen(libPath,RTLD_LAZY);
	if (!gtkLib)
	{
		LOG(Warn,"Failed to load the GTK UI - " + std::string(dlerror()));
		return false;
	}

	BIND_FUNCTION(gtkLib,update_dialog_gtk_new);
	BIND_FUNCTION(gtkLib,update_dialog_gtk_exec);
	BIND_FUNCTION(gtkLib,update_dialog_gtk_handle_error);
	BIND_FUNCTION(gtkLib,update_dialog_gtk_handle_progress);
	BIND_FUNCTION(gtkLib,update_dialog_gtk_handle_finished);
	
	m_dialog = update_dialog_gtk_new(argc,argv);

	return true;
}

void UpdateDialogGtkWrapper::exec()
{
	update_dialog_gtk_exec(m_dialog);
}

void UpdateDialogGtkWrapper::updateError(const std::string& errorMessage)
{
	update_dialog_gtk_handle_error(m_dialog,errorMessage);
}

void UpdateDialogGtkWrapper::updateProgress(int percentage)
{
	update_dialog_gtk_handle_progress(m_dialog,percentage);
}

void UpdateDialogGtkWrapper::updateFinished()
{
	update_dialog_gtk_handle_finished(m_dialog);
}


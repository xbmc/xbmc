#pragma once

#include "UpdateDialog.h"

#include <fstream>
#include "tinythread.h"

/** A fallback auto-update progress 'dialog' for use on
  * Linux when the GTK UI cannot be loaded.
  *
  * The 'dialog' consists of an xterm tailing the contents
  * of a file, into which progress messages are written.
  */
class UpdateDialogAscii : public UpdateDialog
{
	public:
		// implements UpdateDialog
		virtual void init(int argc, char** argv);
		virtual void exec();
		virtual void quit();

		// implements UpdateObserver
		virtual void updateError(const std::string& errorMessage);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

	private:
		tthread::mutex m_mutex;
		std::ofstream m_output;
};


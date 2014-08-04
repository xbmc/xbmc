#pragma once

#include "UpdateDialog.h"

#include <fstream>
#include "tinythread.h"

class UpdateDialogNull : public UpdateDialog
{
	public:
		virtual void init(int argc, char **argv) {}
		virtual void exec() {}
		virtual void quit() {}
		virtual void updateError(const std::string& errorMessage) {}
		virtual void updateProgress(int percentage) {}
		virtual void updateFinished() {}
};


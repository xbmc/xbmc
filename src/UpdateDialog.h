#pragma once

#include "UpdateObserver.h"

class UpdateDialog : public UpdateObserver
{
	public:
		virtual ~UpdateDialog() {};

		virtual void init(int argc, char** argv) = 0;
		virtual void exec() = 0;
		virtual void quit() = 0;
};


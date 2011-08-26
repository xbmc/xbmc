#pragma once

#include "UpdateObserver.h"

#include <fstream>
#include "tinythread.h"

class UpdateDialogAscii : public UpdateObserver
{
	public:
		void init();

		virtual void updateError(const std::string& errorMessage);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

	private:
		tthread::mutex m_mutex;
		std::ofstream m_output;
};


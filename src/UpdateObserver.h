#pragma once

#include <string>

/** Base class for observers of update installation status.
  * See UpdateInstaller::setObserver()
  */
class UpdateObserver
{
	public:
		virtual void updateError(const std::string& errorMessage) = 0;
		virtual void updateProgress(int percentage) = 0;
		virtual void updateFinished() = 0;
};


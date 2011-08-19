#pragma once

class UpdateObserver
{
	public:
		virtual void updateError(const std::string& errorMessage) = 0;
		virtual bool updateRetryCancel(const std::string& message) = 0;
		virtual void updateProgress(int percentage) = 0;
		virtual void updateFinished() = 0;
};


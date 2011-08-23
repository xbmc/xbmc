#pragma once

#include "UpdateObserver.h"

class UpdateDialogPrivate;

class UpdateDialogCocoa : public UpdateObserver
{
	public:
		UpdateDialogCocoa();
		~UpdateDialogCocoa();

		void init();
		void exec();

		// implements UpdateObserver
		virtual void updateError(const std::string& errorMessage);
		virtual bool updateRetryCancel(const std::string& message);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

	private:
		UpdateDialogPrivate* d;
};


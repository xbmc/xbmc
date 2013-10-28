#pragma once

#include "UpdateDialog.h"
#include "UpdateObserver.h"

class UpdateDialogPrivate;

class UpdateDialogCocoa : public UpdateDialog
{
	public:
		UpdateDialogCocoa();
		~UpdateDialogCocoa();

		// implements UpdateDialog
		virtual void init(int argc, char** argv);
		virtual void exec();
		virtual void quit();

		// implements UpdateObserver
		virtual void updateError(const std::string& errorMessage);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

		static void* createAutoreleasePool();
		static void releaseAutoreleasePool(void* data);

	private:
		void enableDockIcon();

		UpdateDialogPrivate* d;
};


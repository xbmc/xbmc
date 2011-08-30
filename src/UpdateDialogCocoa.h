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
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

		static void* createAutoreleasePool();
		static void releaseAutoreleasePool(void* data);

	private:
		void enableDockIcon();

		UpdateDialogPrivate* d;
};


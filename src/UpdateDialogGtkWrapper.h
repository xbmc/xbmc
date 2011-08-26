#pragma once

#include "UpdateObserver.h"

class UpdateDialogGtk;

/** A wrapper around UpdateDialogGtk which allows the GTK UI to
  * be loaded dynamically at runtime if the GTK libraries are
  * available.
  */
class UpdateDialogGtkWrapper : public UpdateObserver
{
	public:
		UpdateDialogGtkWrapper();

		/** Attempt to load and initialize the GTK updater UI.
		  * If this function returns false, other calls in UpdateDialogGtkWrapper
		  * may not be used.
		  */
		bool init(int argc, char** argv);
		void exec();

		virtual void updateError(const std::string& errorMessage);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

	private:
		UpdateDialogGtk* m_dialog;
};


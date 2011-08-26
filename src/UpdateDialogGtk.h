#pragma once

#include "UpdateMessage.h"
#include "UpdateObserver.h"

#include <gtk/gtk.h>

class UpdateDialogGtk : public UpdateObserver
{
	public:
		UpdateDialogGtk();

		void init(int argc, char** argv);
		void exec();

		// observer callbacks - these may be called
		// from a background thread
		virtual void updateError(const std::string& errorMessage);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

	private:
		static void finish(GtkWidget* widget, gpointer dialog);
		static gboolean notify(void* message);

		GtkWidget* m_window;
		GtkWidget* m_progressLabel;
		GtkWidget* m_finishButton;
		GtkWidget* m_progressBar;
		bool m_hadError;
};


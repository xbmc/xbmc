#pragma once

#include "UpdateDialog.h"
#include "UpdateMessage.h"
#include "UpdateObserver.h"

#include <gtk/gtk.h>

class UpdateDialogGtk : public UpdateDialog
{
	public:
		UpdateDialogGtk();

		// implements UpdateDialog
		virtual void init(int argc, char** argv);
		virtual void exec();
		virtual void quit();

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

// helper functions which allow the GTK dialog to be loaded dynamically
// at runtime and used only if the GTK libraries are actually present
extern "C" {
	UpdateDialogGtk* update_dialog_gtk_new();
}



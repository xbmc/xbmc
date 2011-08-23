#pragma once

#include "UpdateObserver.h"

#include <gtk/gtk.h>

class UpdateDialogGtk : public UpdateObserver
{
	public:
		UpdateDialogGtk();

		bool restartApp() const;

		void init(int argc, char** argv);
		void exec();

		// observer callbacks - these may be called
		// from a background thread
		virtual void updateError(const std::string& errorMessage);
		virtual bool updateRetryCancel(const std::string& message);
		virtual void updateProgress(int percentage);
		virtual void updateFinished();

	private:
		struct Message
		{
			enum Type
			{
				UpdateFailed,
				UpdateProgress,
				UpdateFinished
			};

			Message(UpdateDialogGtk* _dialog, Type _type)
			: dialog(_dialog)
			, type(_type)
			{
			}

			UpdateDialogGtk* dialog;
			Type type;
			std::string message;
			int progress;
		};

		static void finish(void* dialog);
		static gboolean notify(void* message);

		GtkWidget* m_window;
		GtkWidget* m_progressLabel;
		GtkWidget* m_finishButton;
		GtkWidget* m_progressBar;
		bool m_restartApp;
};


#include "UpdateDialogGtk.h"

#include "AppInfo.h"
#include "StringUtils.h"

#include <glib.h>
#include <gtk/gtk.h>

UpdateDialogGtk* update_dialog_gtk_new()
{
	return new UpdateDialogGtk();
}

UpdateDialogGtk::UpdateDialogGtk()
: m_hadError(false)
{
}

void UpdateDialogGtk::init(int argc, char** argv)
{
	gtk_init(&argc,&argv);

	m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(m_window),AppInfo::name().c_str());

	m_progressLabel = gtk_label_new("Installing Updates");

	GtkWidget* windowLayout = gtk_vbox_new(FALSE,3);
	GtkWidget* buttonLayout = gtk_hbox_new(FALSE,3);
	GtkWidget* labelLayout = gtk_hbox_new(FALSE,3);

	m_finishButton = gtk_button_new_with_label("Finish");
	gtk_widget_set_sensitive(m_finishButton,false);

	m_progressBar = gtk_progress_bar_new();

	// give the dialog a sensible default size by setting a minimum
	// width on the progress bar.  This is used instead of setting
	// a default size for the dialog since gtk_window_set_default_size()
	// is ignored when a dialog is marked as non-resizable
	gtk_widget_set_usize(m_progressBar,350,-1);

	gtk_signal_connect(GTK_OBJECT(m_finishButton),"clicked",
	                   GTK_SIGNAL_FUNC(UpdateDialogGtk::finish),this);

	gtk_container_add(GTK_CONTAINER(m_window),windowLayout);
	gtk_container_set_border_width(GTK_CONTAINER(m_window),12);

	gtk_box_pack_start(GTK_BOX(labelLayout),m_progressLabel,false,false,0);
	gtk_box_pack_end(GTK_BOX(buttonLayout),m_finishButton,false,false,0);

	gtk_box_pack_start(GTK_BOX(windowLayout),labelLayout,false,false,0);
	gtk_box_pack_start(GTK_BOX(windowLayout),m_progressBar,false,false,0);
	gtk_box_pack_start(GTK_BOX(windowLayout),buttonLayout,false,false,0);

	
	gtk_widget_show(m_progressLabel);
	gtk_widget_show(labelLayout);
	gtk_widget_show(windowLayout);
	gtk_widget_show(buttonLayout);
	gtk_widget_show(m_finishButton);
	gtk_widget_show(m_progressBar);

	gtk_window_set_resizable(GTK_WINDOW(m_window),false);
	gtk_window_set_position(GTK_WINDOW(m_window),GTK_WIN_POS_CENTER);

	gtk_widget_show(m_window);
}

void UpdateDialogGtk::exec()
{
	gtk_main();
}

void UpdateDialogGtk::finish(GtkWidget* widget, gpointer _dialog)
{
	UpdateDialogGtk* dialog = static_cast<UpdateDialogGtk*>(_dialog);
	dialog->quit();
}

void UpdateDialogGtk::quit()
{
	gtk_main_quit();
}

gboolean UpdateDialogGtk::notify(void* _message)
{
	UpdateMessage* message = static_cast<UpdateMessage*>(_message);
	UpdateDialogGtk* dialog = static_cast<UpdateDialogGtk*>(message->receiver);
	switch (message->type)
	{
		case UpdateMessage::UpdateProgress:
			gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(dialog->m_progressBar),message->progress/100.0);
			break;
		case UpdateMessage::UpdateFailed:
			{
				dialog->m_hadError = true;
				std::string errorMessage = AppInfo::updateErrorMessage(message->message);
				GtkWidget* errorDialog = gtk_message_dialog_new (GTK_WINDOW(dialog->m_window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						"%s",
						errorMessage.c_str());
				gtk_dialog_run (GTK_DIALOG (errorDialog));
				gtk_widget_destroy (errorDialog);
				gtk_widget_set_sensitive(dialog->m_finishButton,true);
			}
			break;
		case UpdateMessage::UpdateFinished:
			{
				std::string message;
				if (dialog->m_hadError)
				{
					message = "Update failed.";
				}
				else
				{
					message = "Update installed.";
				}
				message += "  Click 'Finish' to restart the application.";
				gtk_label_set_text(GTK_LABEL(dialog->m_progressLabel),message.c_str());
				gtk_widget_set_sensitive(dialog->m_finishButton,true);
			}
			break;
	}
	delete message;

	// do not invoke this function again
	return false;
}

// callbacks during update installation
void UpdateDialogGtk::updateError(const std::string& errorMessage)
{
	UpdateMessage* message = new UpdateMessage(this,UpdateMessage::UpdateFailed);
	message->message = errorMessage;
	g_idle_add(&UpdateDialogGtk::notify,message);
}

void UpdateDialogGtk::updateProgress(int percentage)
{
	UpdateMessage* message = new UpdateMessage(this,UpdateMessage::UpdateProgress);
	message->progress = percentage;
	g_idle_add(&UpdateDialogGtk::notify,message);
}

void UpdateDialogGtk::updateFinished()
{
	UpdateMessage* message = new UpdateMessage(this,UpdateMessage::UpdateFinished);
	g_idle_add(&UpdateDialogGtk::notify,message);
	UpdateDialog::updateFinished();
}



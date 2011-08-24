#include "UpdateDialogGtk.h"

#include "Log.h"
#include "StringUtils.h"

#include <glib.h>
#include <gtk/gtk.h>

UpdateDialogGtk::UpdateDialogGtk()
: m_restartApp(false)
{
}

bool UpdateDialogGtk::restartApp() const
{
	return m_restartApp;
}

void UpdateDialogGtk::init(int argc, char** argv)
{
	gtk_init(&argc,&argv);

	m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(m_window),"Mendeley Updater");
	gtk_window_set_resizable(GTK_WINDOW(m_window),false);

	m_progressLabel = gtk_label_new("Installing Updates");
	GtkWidget* windowLayout = gtk_vbox_new(FALSE,3);
	GtkWidget* buttonLayout = gtk_hbox_new(FALSE,3);
	GtkWidget* labelLayout = gtk_hbox_new(FALSE,3);

	m_finishButton = gtk_button_new_with_label("Finish");
	gtk_widget_set_sensitive(m_finishButton,false);

	m_progressBar = gtk_progress_bar_new();

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
	dialog->m_restartApp = true;
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
			gtk_label_set_text(GTK_LABEL(dialog->m_progressLabel),
			  ("There was a problem installing the update: " + message->message).c_str());;
			gtk_widget_set_sensitive(dialog->m_finishButton,true);
			break;
		case UpdateMessage::UpdateFinished:
			gtk_label_set_text(GTK_LABEL(dialog->m_progressLabel),
			  "Update installed.  Click 'Finish' to restart the application.");
			gtk_widget_set_sensitive(dialog->m_finishButton,true);
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

bool UpdateDialogGtk::updateRetryCancel(const std::string& message)
{
	// TODO
	return false;
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
}



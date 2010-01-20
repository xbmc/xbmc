#include <gtk/gtk.h>


void
on_checkbutton_double_toggled          (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_spinbutton_fps_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_combo_entry_winsize_changed         (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_config_window_destroy_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_config_window_delete_event          (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "gtk-callbacks.h"
#include "gtk-interface.h"
#include "gtk-support.h"

#define WINSIZE_COMBO "combo_winsize"

#include "sdl_goom.h"
#include "goom_config_param.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SdlGoom *sdlGoom;
static GtkObject *owin;

static GdkColor color_blue;
static GdkColor color_red;

void highlight_buffer(GtkText *editable)
{
    static int last_len = 0;
    int i, len;
    int next_add = 0;
   
    return;
    
    gint save_pos = gtk_editable_get_position(GTK_EDITABLE(editable)); /* save current pos */
    gchar *txt  = gtk_editable_get_chars (GTK_EDITABLE(editable),0,-1);
    len = strlen(txt);
    if (len == 0) return;

    if (len == last_len) return;
    last_len = len;
    
    gtk_text_freeze(editable);

    gtk_text_set_point(editable,0);
    gtk_text_forward_delete(editable,len);

    for (i=0;i<len-1;++i) {
        if ((txt[i]=='/')&&(txt[i+1]=='*')) {
            if (i>next_add) {
                gtk_text_insert (editable, 0, 0, 0, txt+next_add, i-next_add);
                next_add = i;
            }
        }
        if ((txt[i]=='*')&&(txt[i+1]=='/')) {
            if (i>next_add) {
                gtk_text_insert (editable, 0, &color_blue, 0, txt+next_add, i-next_add+2);
                next_add = i + 2;
            }
        }
    }
    if (next_add < len) {
        gtk_text_insert (editable, 0, 0, 0, txt+next_add, len-next_add);
    }

    /*    gtk_editable_set_position(GTK_EDITABLE(editable),save_pos+40);
          gtk_editable_set_position(GTK_EDITABLE(editable),save_pos-40); */
    
    gtk_editable_set_position(GTK_EDITABLE(editable),save_pos); /* restore current pos */
    gtk_text_thaw(editable);
    g_free(txt);
}    

void
on_spinbutton_int_changed              (GtkEditable     *editable,
                                        gpointer         user_data)
{
  PluginParam *param = (PluginParam*)gtk_object_get_data (GTK_OBJECT(editable),"param");
  IVAL(*param) = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(editable));
  param->changed(param);
}

void
on_adj_float_changed              (GtkWidget     *w,
                                   gpointer         user_data)
{
	PluginParam *param = (PluginParam*)gtk_object_get_data (GTK_OBJECT(w),"param");
	FVAL(*param) = GTK_ADJUSTMENT(w)->value;
	param->changed(param);
} 

void
on_text_changed         (GtkEditable     *editable,
			 gpointer         user_data)
{
  PluginParam *param = (PluginParam*)gtk_object_get_data (GTK_OBJECT(editable),"param");
  gchar *txt = gtk_editable_get_chars (editable,0,-1);
  set_str_param_value(param, txt);
  param->changed(param);
  g_free(txt);
  highlight_buffer(GTK_TEXT(editable));
}

void
on_list_changed         (GtkEditable     *editable,
			 gpointer         user_data)
{
  PluginParam *param = (PluginParam*)gtk_object_get_data (GTK_OBJECT(editable),"param");
  gchar *txt = gtk_editable_get_chars (editable,0,-1);
  set_list_param_value(param, txt);
  param->changed(param);
  g_free(txt);
}

void
on_bool_toggled          (GtkToggleButton *togglebutton,
			  gpointer         user_data)
{
  PluginParam *param = (PluginParam*)gtk_object_get_data (GTK_OBJECT(togglebutton),"param");
  BVAL(*param) = gtk_toggle_button_get_active(togglebutton);
  param->changed(param);
}

void my_int_listener (PluginParam *param) {
  GtkEditable *editable;

  if (sdlGoom->config_win == 0) return;
  editable = GTK_EDITABLE(param->user_data);

  if (editable) {
    int pos = 0;
    char str[256];
    sprintf (str, "%d", IVAL(*param));
    if (strcmp(str,gtk_editable_get_chars(editable,0,-1))) {
      gtk_editable_delete_text (editable,0,-1);
      gtk_editable_insert_text (editable,str,strlen(str),&pos);
    }
  }
}

void my_list_listener (PluginParam *param) {
  GtkEntry *editable;

  if (sdlGoom->config_win == 0) return;
  editable = GTK_ENTRY(param->user_data);

  if (editable) {
    if (strcmp(gtk_entry_get_text(editable),LVAL(*param))) {
      gtk_entry_set_text (editable, LVAL(*param));
    }
  }
}

void my_bool_listener (PluginParam *param) {
  GtkCheckButton *editable;

  if (sdlGoom->config_win == 0) return;
  editable = GTK_CHECK_BUTTON(param->user_data);

  if (editable) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(editable)) != BVAL(*param))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(editable),BVAL(*param));
  }
}

void my_float_listener_progress (PluginParam *param) {
  GtkProgressBar *progress;

  if (sdlGoom->config_win == 0) return;
  progress = GTK_PROGRESS_BAR(param->user_data);

  if (progress) {
    if (FVAL(*param)<FMIN(*param))
      FVAL(*param) = FMIN(*param);
    if (FVAL(*param)>FMAX(*param))
      FVAL(*param) = FMAX(*param);
    gtk_progress_bar_update (progress, FVAL(*param));
  }
}

void my_float_listener_scale (PluginParam *param) {
  GtkRange *range;

  if (sdlGoom->config_win == 0) return;
  range = GTK_RANGE(param->user_data);

  if (range) {
	  GtkAdjustment *adj;
    if (FVAL(*param) < FMIN(*param))
      FVAL(*param) = FMIN(*param);
    if (FVAL(*param) > FMAX(*param))
      FVAL(*param) = FMAX(*param);

    adj = gtk_range_get_adjustment(range);
    adj->value = FVAL(*param);
    gtk_adjustment_value_changed(adj);
//  gtk_range_set_adjustment(range, adj);
  }
}

void addParams (GtkNotebook *notebook, PluginParameters *params) {
  int n;
  GtkWidget *table = gtk_table_new (params->nbParams, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 11);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);

  for (n=0;n<params->nbParams;++n) {
    if (params->params[n] == 0) {
      GtkWidget *hseparator = gtk_hseparator_new ();
      gtk_widget_show (hseparator);
      gtk_table_attach (GTK_TABLE (table), hseparator, 0, 2, n, n+1,
			(GtkAttachOptions) (GTK_FILL),
			(GtkAttachOptions) (GTK_FILL), 0, 5);
    } else {
      PluginParam *p = params->params[n];
      int type = p->type;
      
      if (type != PARAM_BOOLVAL) {
	GtkWidget *label4 = gtk_label_new (p->name);
	gtk_widget_show (label4);
	gtk_table_attach (GTK_TABLE (table), label4, 0, 1, n, n+1,
			  (GtkAttachOptions) (GTK_FILL),
			  (GtkAttachOptions) (0), 0, 0);
	gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment (GTK_MISC (label4), 0, 0.5);
      }

      switch (type) {
	case PARAM_INTVAL: {
	  GtkWidget *spinbutton_adj,*spinbutton;

	  spinbutton_adj = (GtkWidget*)gtk_adjustment_new (
	    p->param.ival.value,
	    p->param.ival.min, p->param.ival.max,
	    p->param.ival.step, p->param.ival.step*10,
	    p->param.ival.step*10);
	  spinbutton = gtk_spin_button_new (GTK_ADJUSTMENT (spinbutton_adj), 1, 0);
	  gtk_widget_show (spinbutton);
	  gtk_table_attach (GTK_TABLE (table), spinbutton, 1, 2, n, n+1,
			    (GtkAttachOptions) (GTK_FILL),
			    (GtkAttachOptions) (0), 0, 1);
	  gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinbutton), TRUE);
	  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (spinbutton),
					     GTK_UPDATE_IF_VALID);
	  p->user_data = spinbutton;
	  gtk_object_set_data (GTK_OBJECT(spinbutton),"param",(void*)p);
	  p->change_listener = my_int_listener;
	  gtk_signal_connect (GTK_OBJECT (spinbutton), "changed",
			      GTK_SIGNAL_FUNC (on_spinbutton_int_changed),
			      NULL);
	  break;
	}

	case PARAM_FLOATVAL: {
	  GtkWidget *progress,*prog_adj;

	  prog_adj = (GtkWidget*)gtk_adjustment_new (
	    p->param.fval.value,
	    p->param.fval.min, p->param.fval.max,
	    p->param.fval.step, p->param.fval.step*10,
	    p->param.fval.step*10);

	  if (p->rw)
		  progress = gtk_hscale_new(GTK_ADJUSTMENT(prog_adj));
	  else
		  progress = gtk_progress_bar_new_with_adjustment(GTK_ADJUSTMENT(prog_adj));
	  gtk_widget_show(progress);
	  gtk_table_attach (GTK_TABLE (table), progress, 1, 2, n, n+1,
			    (GtkAttachOptions) (GTK_FILL),
			    (GtkAttachOptions) (0), 0, 1);
	  
	  p->user_data = progress;
	  if (p->rw) {
		  p->change_listener = my_float_listener_scale;
	  	  gtk_object_set_data (GTK_OBJECT(prog_adj),"param",(void*)p);
		  gtk_signal_connect (GTK_OBJECT (prog_adj), "value-changed",
			      GTK_SIGNAL_FUNC (on_adj_float_changed),
			      NULL);
	  }
	  else
		  p->change_listener = my_float_listener_progress;
	  break;
	}

	case PARAM_STRVAL: {
          GtkWidget *vscrollbar = 0;
	  GtkWidget *text_winsize = gtk_text_new (0,0);
	  gtk_widget_show (text_winsize);
	  gtk_table_attach (GTK_TABLE (table), text_winsize, 1, 2, n, n+1,
			    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
	  gtk_text_insert (GTK_TEXT(text_winsize), 0, 0, 0, p->param.sval.value, strlen(p->param.sval.value));
          gtk_text_set_editable(GTK_TEXT(text_winsize), p->rw);
          /* Add a vertical scrollbar to the GtkText widget */
          vscrollbar = gtk_vscrollbar_new (GTK_TEXT (text_winsize)->vadj);
          gtk_table_attach (GTK_TABLE (table), vscrollbar, 2, 3, n, n+1,
                  GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
          gtk_widget_show (vscrollbar);

/*	  combo_entry_winsize = GTK_COMBO (combo_winsize)->entry;
	  gtk_widget_show (combo_entry_winsize);
	  gtk_entry_set_editable (GTK_ENTRY (combo_entry_winsize), FALSE);
	  gtk_entry_set_text (GTK_ENTRY (combo_entry_winsize), LVAL(*p));
	  p->change_listener = my_list_listener;*/
	  p->user_data = text_winsize;
	  gtk_object_set_data (GTK_OBJECT(text_winsize),"param",(void*)p);
	  gtk_signal_connect (GTK_OBJECT (text_winsize), "changed",
			      GTK_SIGNAL_FUNC (on_text_changed),
			      NULL);
	  break;
	}

	case PARAM_LISTVAL: {
	  int i;
	  GList *combo_winsize_items = NULL;
	  GtkWidget *combo_entry_winsize = NULL;
	  GtkWidget *combo_winsize = gtk_combo_new ();
	  gtk_widget_show (combo_winsize);
	  gtk_table_attach (GTK_TABLE (table), combo_winsize, 1, 2, n, n+1,
			    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
			    (GtkAttachOptions) (0), 0, 0);
	  gtk_combo_set_value_in_list (GTK_COMBO (combo_winsize), TRUE, TRUE);
	  for (i=0;i<p->param.slist.nbChoices;++i)
	    combo_winsize_items = g_list_append (combo_winsize_items,
						 p->param.slist.choices[i]);
	  gtk_combo_set_popdown_strings (GTK_COMBO (combo_winsize), combo_winsize_items);
	  g_list_free (combo_winsize_items);
	  
	  combo_entry_winsize = GTK_COMBO (combo_winsize)->entry;
	  gtk_widget_show (combo_entry_winsize);
	  gtk_entry_set_editable (GTK_ENTRY (combo_entry_winsize), FALSE);
	  gtk_entry_set_text (GTK_ENTRY (combo_entry_winsize), LVAL(*p));
	  p->change_listener = my_list_listener;
	  p->user_data = combo_entry_winsize;
	  gtk_object_set_data (GTK_OBJECT(combo_entry_winsize),"param",(void*)p);
	  gtk_signal_connect (GTK_OBJECT (combo_entry_winsize), "changed",
			      GTK_SIGNAL_FUNC (on_list_changed),
			      NULL);
	  break;
	}

	case PARAM_BOOLVAL: {
	  GtkWidget *checkbutton_double =
	    gtk_check_button_new_with_label (p->name);
	  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(checkbutton_double),BVAL(*p));
	  gtk_widget_show (checkbutton_double);
	  gtk_table_attach (GTK_TABLE (table), checkbutton_double, 1, 2, n, n+1,
			    (GtkAttachOptions) (GTK_FILL),
			    (GtkAttachOptions) (0), 0, 0);
	  gtk_signal_connect (GTK_OBJECT (checkbutton_double), "toggled",
			      GTK_SIGNAL_FUNC (on_bool_toggled),
			      NULL);
	  gtk_object_set_data (GTK_OBJECT(checkbutton_double),"param",(void*)p);
	  p->user_data = checkbutton_double;
	  p->change_listener = my_bool_listener;
	  break;
	}
      }
    }
  }

  gtk_widget_show_all(GTK_WIDGET(table));
  gtk_container_add(GTK_CONTAINER(notebook),table);
  gtk_notebook_set_tab_label_text(notebook,GTK_WIDGET(table),params->name);
}

void gtk_data_init(SdlGoom *sg) {

  sdlGoom = sg;
  if (sdlGoom->config_win) {
    int i;
    GtkNotebook *notebook;
    GdkColormap *cmap;

    cmap = gdk_colormap_get_system();
    color_blue.red = 0;
    color_blue.blue = 0xffff;
    color_blue.green = 0;
    color_red.red = 0xffff;
    color_red.blue = 0;
    color_red.green = 0;
    if ((!gdk_color_alloc(cmap, &color_blue))||(!gdk_color_alloc(cmap, &color_red))) {
        g_error("couldn't allocate color");
    }
    
    owin = GTK_OBJECT(sdlGoom->config_win);
    notebook = GTK_NOTEBOOK(gtk_object_get_data (owin, "notebook1")); 
    addParams (notebook, &sdlGoom->screen);
    for (i = 0; i < sdlGoom->plugin->nbParams; ++i) {
      addParams (notebook, &sdlGoom->plugin->params[i]);
    }
  }

}

gboolean
on_config_window_destroy_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  sdlGoom->config_win = 0;
  owin = 0;
  return FALSE;
}


gboolean
on_config_window_delete_event          (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
  sdlGoom->config_win = 0;
  owin = 0;
  return FALSE;
}


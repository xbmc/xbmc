/* 
   Unix SMB/CIFS implementation.
   SMB client GTK+ tree-based application
   Copyright (C) Andrew Tridgell 1998
   Copyright (C) Richard Sharpe 2001
   Copyright (C) John Terpstra 2001
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* example-gtk+ application, ripped off from the gtk+ tree.c sample */

#include <stdio.h>
#include <errno.h>
#include <gtk/gtk.h>
#include "libsmbclient.h"

static GtkWidget *clist;

struct tree_data {

  guint32 type;    /* Type of tree item, an SMBC_TYPE */
  char name[256];  /* May need to change this later   */

};

static void tree_error_message(gchar *message) {

  GtkWidget *dialog, *label, *okay_button;
     
  /* Create the widgets */
     
  dialog = gtk_dialog_new();
  gtk_window_set_modal(GTK_WINDOW(dialog), True);
  label = gtk_label_new (message);
  okay_button = gtk_button_new_with_label("Okay");
     
  /* Ensure that the dialog box is destroyed when the user clicks ok. */
     
  gtk_signal_connect_object (GTK_OBJECT (okay_button), "clicked",
			     GTK_SIGNAL_FUNC (gtk_widget_destroy), dialog);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area),
		     okay_button);

  /* Add the label, and show everything we've added to the dialog. */

  gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
		     label);
  gtk_widget_show_all (dialog);
}

/*
 * We are given a widget, and we want to retrieve its URL so we 
 * can do a directory listing.
 *
 * We walk back up the tree, picking up pieces until we hit a server or
 * workgroup type and return a path from there
 */

static pstring path_string;

char *get_path(GtkWidget *item)
{
  GtkWidget *p = item;
  struct tree_data *pd;
  char *comps[1024];  /* We keep pointers to the components here */
  int i = 0, j, level,type;

  /* Walk back up the tree, getting the private data */

  level = GTK_TREE(item->parent)->level;

  /* Pick up this item's component info */

  pd = (struct tree_data *)gtk_object_get_user_data(GTK_OBJECT(item));

  comps[i++] = pd->name;
  type = pd->type;

  while (level > 0 && type != SMBC_SERVER && type != SMBC_WORKGROUP) {

    /* Find the parent and extract the data etc ... */

    p = GTK_WIDGET(p->parent);    
    p = GTK_WIDGET(GTK_TREE(p)->tree_owner);

    pd = (struct tree_data *)gtk_object_get_user_data(GTK_OBJECT(p));

    level = GTK_TREE(item->parent)->level;

    comps[i++] = pd->name;
    type = pd->type;

  }

  /* 
   * Got a list of comps now, should check that we did not hit a workgroup
   * when we got other things as well ... Later
   *
   * Now, build the path
   */

  pstrcpy( path_string, "smb:/" );

  for (j = i - 1; j >= 0; j--) {

    strncat(path_string, "/", sizeof(path_string) - strlen(path_string));
    strncat(path_string, comps[j], sizeof(path_string) - strlen(path_string));

  }
  
  fprintf(stdout, "Path string = %s\n", path_string);

  return path_string;

}

struct tree_data *make_tree_data(guint32 type, const char *name)
{
  struct tree_data *p = SMB_MALLOC_P(struct tree_data);

  if (p) {

    p->type = type;
    strncpy(p->name, name, sizeof(p->name));

  }

  return p;

}

/* Note that this is called every time the user clicks on an item,
   whether it is already selected or not. */
static void cb_select_child (GtkWidget *root_tree, GtkWidget *child,
			     GtkWidget *subtree)
{
  gint dh, err, dirlen;
  char dirbuf[512];
  struct smbc_dirent *dirp;
  struct stat st1;
  pstring path, path1;

  g_print ("select_child called for root tree %p, subtree %p, child %p\n",
	   root_tree, subtree, child);

  /* Now, figure out what it is, and display it in the clist ... */

  gtk_clist_clear(GTK_CLIST(clist));  /* Clear the CLIST */

  /* Now, get the private data for the subtree */

  strncpy(path, get_path(child), 1024);

  if ((dh = smbc_opendir(path)) < 0) { /* Handle error */

    g_print("cb_select_child: Could not open dir %s, %s\n", path,
	    strerror(errno));

    gtk_main_quit();

    return;

  }

  while ((err = smbc_getdents(dh, (struct smbc_dirent *)dirbuf,
			      sizeof(dirbuf))) != 0) {

    if (err < 0) {

      g_print("cb_select_child: Could not read dir %s, %s\n", path,
	      strerror(errno));

      gtk_main_quit();

      return;

    }

    dirp = (struct smbc_dirent *)dirbuf;

    while (err > 0) {
      gchar col1[128], col2[128], col3[128], col4[128];
      gchar *rowdata[4] = {col1, col2, col3, col4};

      dirlen = dirp->dirlen;

      /* Format each of the items ... */

      strncpy(col1, dirp->name, 128);

      col2[0] = col3[0] = col4[0] = (char)0;

      switch (dirp->smbc_type) {

      case SMBC_WORKGROUP:

	break;

      case SMBC_SERVER:

	strncpy(col2, (dirp->comment?dirp->comment:""), 128);

	break;

      case SMBC_FILE_SHARE:

	strncpy(col2, (dirp->comment?dirp->comment:""), 128);

	break;

      case SMBC_PRINTER_SHARE:

	strncpy(col2, (dirp->comment?dirp->comment:""), 128);
	break;

      case SMBC_COMMS_SHARE:

	break;

      case SMBC_IPC_SHARE:

	break;

      case SMBC_DIR:
      case SMBC_FILE:

	/* Get stats on the file/dir and see what we have */

	if ((strcmp(dirp->name, ".") != 0) &&
	    (strcmp(dirp->name, "..") != 0)) {

	  strncpy(path1, path, sizeof(path1));
	  strncat(path1, "/", sizeof(path) - strlen(path));
	  strncat(path1, dirp->name, sizeof(path) - strlen(path));

	  if (smbc_stat(path1, &st1) < 0) {
	    
	    if (errno != EBUSY) {
	      
	      g_print("cb_select_child: Could not stat file %s, %s\n", path1, 
		      strerror(errno));
	    
	      gtk_main_quit();

	      return;

	    }
	    else {

	      strncpy(col2, "Device or resource busy", sizeof(col2));

	    }
	  }
	  else {
	    /* Now format each of the relevant things ... */

	    snprintf(col2, sizeof(col2), "%c%c%c%c%c%c%c%c%c(%0X)",
		     (st1.st_mode&S_IRUSR?'r':'-'),
		     (st1.st_mode&S_IWUSR?'w':'-'),
		     (st1.st_mode&S_IXUSR?'x':'-'),
		     (st1.st_mode&S_IRGRP?'r':'-'),
		     (st1.st_mode&S_IWGRP?'w':'-'),
		     (st1.st_mode&S_IXGRP?'x':'-'),
		     (st1.st_mode&S_IROTH?'r':'-'),
		     (st1.st_mode&S_IWOTH?'w':'-'),
		     (st1.st_mode&S_IXOTH?'x':'-'),
		     st1.st_mode); 
	    snprintf(col3, sizeof(col3), "%u", st1.st_size);
	    snprintf(col4, sizeof(col4), "%s", ctime(&st1.st_mtime));
	  }
	}

	break;

      default:

	break;
      }

      gtk_clist_append(GTK_CLIST(clist), rowdata);

      (char *)dirp += dirlen;
      err -= dirlen;

    }

  }

}

/* Note that this is never called */
static void cb_unselect_child( GtkWidget *root_tree,
                               GtkWidget *child,
                               GtkWidget *subtree )
{
  g_print ("unselect_child called for root tree %p, subtree %p, child %p\n",
	   root_tree, subtree, child);
}

/* for all the GtkItem:: and GtkTreeItem:: signals */
static void cb_itemsignal( GtkWidget *item,
                           gchar     *signame )
{
  GtkWidget *real_tree, *aitem, *subtree;
  gchar *name;
  GtkLabel *label;
  gint dh, err, dirlen, level;
  char dirbuf[512];
  struct smbc_dirent *dirp;
  
  label = GTK_LABEL (GTK_BIN (item)->child);
  /* Get the text of the label */
  gtk_label_get (label, &name);

  level = GTK_TREE(item->parent)->level;

  /* Get the level of the tree which the item is in */
  g_print ("%s called for item %s->%p, level %d\n", signame, name,
	   item, GTK_TREE (item->parent)->level);

  real_tree = GTK_TREE_ITEM_SUBTREE(item);  /* Get the subtree */

  if (strncmp(signame, "expand", 6) == 0) { /* Expand called */
    char server[128];

    if ((dh = smbc_opendir(get_path(item))) < 0) { /* Handle error */
      gchar errmsg[256];

      g_print("cb_itemsignal: Could not open dir %s, %s\n", get_path(item), 
	      strerror(errno));

      slprintf(errmsg, sizeof(errmsg), "cb_itemsignal: Could not open dir %s, %s\n", get_path(item), strerror(errno));

      tree_error_message(errmsg);

      /*      gtk_main_quit();*/

      return;

    }

    while ((err = smbc_getdents(dh, (struct smbc_dirent *)dirbuf, 
				sizeof(dirbuf))) != 0) {

      if (err < 0) { /* An error, report it */
	gchar errmsg[256];

	g_print("cb_itemsignal: Could not read dir smbc://, %s\n",
		strerror(errno));

	slprintf(errmsg, sizeof(errmsg), "cb_itemsignal: Could not read dir smbc://, %s\n", strerror(errno));

	tree_error_message(errmsg);

	/*	gtk_main_quit();*/

	return;

      }

      dirp = (struct smbc_dirent *)dirbuf;

      while (err > 0) {
	struct tree_data *my_data;

	dirlen = dirp->dirlen;

	my_data = make_tree_data(dirp->smbc_type, dirp->name);

	if (!my_data) {

	  g_print("Could not allocate space for tree_data: %s\n",
		  dirp->name);

	  gtk_main_quit();
	  return;

	}

	aitem = gtk_tree_item_new_with_label(dirp->name);

	/* Connect all GtkItem:: and GtkTreeItem:: signals */
	gtk_signal_connect (GTK_OBJECT(aitem), "select",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "select");
	gtk_signal_connect (GTK_OBJECT(aitem), "deselect",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "deselect");
	gtk_signal_connect (GTK_OBJECT(aitem), "toggle",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "toggle");
	gtk_signal_connect (GTK_OBJECT(aitem), "expand",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "expand");
	gtk_signal_connect (GTK_OBJECT(aitem), "collapse",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "collapse");
	/* Add it to the parent tree */
	gtk_tree_append (GTK_TREE(real_tree), aitem);

	gtk_widget_show (aitem);

	gtk_object_set_user_data(GTK_OBJECT(aitem), (gpointer)my_data);

	fprintf(stdout, "Added: %s, len: %u\n", dirp->name, dirlen);

	if (dirp->smbc_type != SMBC_FILE &&
	    dirp->smbc_type != SMBC_IPC_SHARE &&
	    (strcmp(dirp->name, ".") != 0) && 
	    (strcmp(dirp->name, "..") !=0)){
	  
	  subtree = gtk_tree_new();
	  gtk_tree_item_set_subtree(GTK_TREE_ITEM(aitem), subtree);

	  gtk_signal_connect(GTK_OBJECT(subtree), "select_child",
			     GTK_SIGNAL_FUNC(cb_select_child), real_tree);
	  gtk_signal_connect(GTK_OBJECT(subtree), "unselect_child",
			     GTK_SIGNAL_FUNC(cb_unselect_child), real_tree);

	}

	(char *)dirp += dirlen;
	err -= dirlen;

      }

    }

    smbc_closedir(dh);   

  }
  else if (strncmp(signame, "collapse", 8) == 0) {
    GtkWidget *subtree = gtk_tree_new();

    gtk_tree_remove_items(GTK_TREE(real_tree), GTK_TREE(real_tree)->children);

    gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), subtree);

    gtk_signal_connect (GTK_OBJECT(subtree), "select_child",
			GTK_SIGNAL_FUNC(cb_select_child), real_tree);
    gtk_signal_connect (GTK_OBJECT(subtree), "unselect_child",
			GTK_SIGNAL_FUNC(cb_unselect_child), real_tree);

  }

}

static void cb_selection_changed( GtkWidget *tree )
{
  GList *i;
  
  g_print ("selection_change called for tree %p\n", tree);
  g_print ("selected objects are:\n");

  i = GTK_TREE_SELECTION(tree);
  while (i){
    gchar *name;
    GtkLabel *label;
    GtkWidget *item;

    /* Get a GtkWidget pointer from the list node */
    item = GTK_WIDGET (i->data);
    label = GTK_LABEL (GTK_BIN (item)->child);
    gtk_label_get (label, &name);
    g_print ("\t%s on level %d\n", name, GTK_TREE
	     (item->parent)->level);
    i = i->next;
  }
}

/*
 * Expand or collapse the whole network ...
 */
static void cb_wholenet(GtkWidget *item, gchar *signame)
{
  GtkWidget *real_tree, *aitem, *subtree;
  gchar *name;
  GtkLabel *label;
  gint dh, err, dirlen;
  char dirbuf[512];
  struct smbc_dirent *dirp;
  
  label = GTK_LABEL (GTK_BIN (item)->child);
  gtk_label_get (label, &name);
  g_print ("%s called for item %s->%p, level %d\n", signame, name,
	   item, GTK_TREE (item->parent)->level);

  real_tree = GTK_TREE_ITEM_SUBTREE(item);  /* Get the subtree */

  if (strncmp(signame, "expand", 6) == 0) { /* Expand called */

    if ((dh = smbc_opendir("smb://")) < 0) { /* Handle error */

      g_print("cb_wholenet: Could not open dir smbc://, %s\n",
	      strerror(errno));

      gtk_main_quit();

      return;

    }

    while ((err = smbc_getdents(dh, (struct smbc_dirent *)dirbuf, 
				sizeof(dirbuf))) != 0) {

      if (err < 0) { /* An error, report it */

	g_print("cb_wholenet: Could not read dir smbc://, %s\n",
		strerror(errno));

	gtk_main_quit();

	return;

      }

      dirp = (struct smbc_dirent *)dirbuf;

      while (err > 0) {
	struct tree_data *my_data;

	dirlen = dirp->dirlen;

	my_data = make_tree_data(dirp->smbc_type, dirp->name);

	aitem = gtk_tree_item_new_with_label(dirp->name);

	/* Connect all GtkItem:: and GtkTreeItem:: signals */
	gtk_signal_connect (GTK_OBJECT(aitem), "select",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "select");
	gtk_signal_connect (GTK_OBJECT(aitem), "deselect",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "deselect");
	gtk_signal_connect (GTK_OBJECT(aitem), "toggle",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "toggle");
	gtk_signal_connect (GTK_OBJECT(aitem), "expand",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "expand");
	gtk_signal_connect (GTK_OBJECT(aitem), "collapse",
			    GTK_SIGNAL_FUNC(cb_itemsignal), "collapse");

	gtk_tree_append (GTK_TREE(real_tree), aitem);
	/* Show it - this can be done at any time */
	gtk_widget_show (aitem);

	gtk_object_set_user_data(GTK_OBJECT(aitem), (gpointer)my_data);

	fprintf(stdout, "Added: %s, len: %u\n", dirp->name, dirlen);

	subtree = gtk_tree_new();

	gtk_tree_item_set_subtree(GTK_TREE_ITEM(aitem), subtree);

	gtk_signal_connect(GTK_OBJECT(subtree), "select_child",
			   GTK_SIGNAL_FUNC(cb_select_child), real_tree);
	gtk_signal_connect(GTK_OBJECT(subtree), "unselect_child",
			   GTK_SIGNAL_FUNC(cb_unselect_child), real_tree);

	(char *)dirp += dirlen;
	err -= dirlen;

      }

    }

    smbc_closedir(dh);   

  }
  else { /* Must be collapse ... FIXME ... */
    GtkWidget *subtree = gtk_tree_new();

    gtk_tree_remove_items(GTK_TREE(real_tree), GTK_TREE(real_tree)->children);

    gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), subtree);

    gtk_signal_connect (GTK_OBJECT(subtree), "select_child",
			GTK_SIGNAL_FUNC(cb_select_child), real_tree);
    gtk_signal_connect (GTK_OBJECT(subtree), "unselect_child",
			GTK_SIGNAL_FUNC(cb_unselect_child), real_tree);


  }

}

/* Should put up a dialog box to ask the user for username and password */

static void 
auth_fn(const char *server, const char *share,
	char *workgroup, int wgmaxlen, char *username, int unmaxlen,
	char *password, int pwmaxlen)
{

   strncpy(username, "test", unmaxlen);
   strncpy(password, "test", pwmaxlen);

}

static char *col_titles[] = {
  "Name", "Attributes", "Size", "Modification Date",
};

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window, *scrolled_win, *scrolled_win2, *tree;
  GtkWidget *subtree, *item, *main_hbox, *r_pane, *l_pane;
  gint err, dh;
  gint i;
  char dirbuf[512];
  struct smbc_dirent *dirp;

  gtk_init (&argc, &argv);

  /* Init the smbclient library */

  err = smbc_init(auth_fn, 10);

  /* Print an error response ... */

  if (err < 0) {

    fprintf(stderr, "smbc_init returned %s (%i)\nDo you have a ~/.smb/smb.conf file?\n", strerror(errno), errno);
    exit(1);

  }

  /* a generic toplevel window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_name(window, "main browser window");
  gtk_signal_connect (GTK_OBJECT(window), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_window_set_title(GTK_WINDOW(window), "The Linux Windows Network Browser");
  gtk_widget_set_usize(GTK_WIDGET(window), 750, -1);
  gtk_container_set_border_width (GTK_CONTAINER(window), 5);

  gtk_widget_show (window);

  /* A container for the two panes ... */

  main_hbox = gtk_hbox_new(FALSE, 1);
  gtk_container_border_width(GTK_CONTAINER(main_hbox), 1);
  gtk_container_add(GTK_CONTAINER(window), main_hbox);

  gtk_widget_show(main_hbox);

  l_pane = gtk_hpaned_new();
  gtk_paned_gutter_size(GTK_PANED(l_pane), (GTK_PANED(l_pane))->handle_size);
  r_pane = gtk_hpaned_new();
  gtk_paned_gutter_size(GTK_PANED(r_pane), (GTK_PANED(r_pane))->handle_size);
  gtk_container_add(GTK_CONTAINER(main_hbox), l_pane);
  gtk_widget_show(l_pane);

  /* A generic scrolled window */
  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize (scrolled_win, 150, 200);
  gtk_container_add (GTK_CONTAINER(l_pane), scrolled_win);
  gtk_widget_show (scrolled_win);

  /* Another generic scrolled window */
  scrolled_win2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win2),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_widget_set_usize (scrolled_win2, 150, 200);
  gtk_paned_add2 (GTK_PANED(l_pane), scrolled_win2);
  gtk_widget_show (scrolled_win2);
  
  /* Create the root tree */
  tree = gtk_tree_new();
  g_print ("root tree is %p\n", tree);
  /* connect all GtkTree:: signals */
  gtk_signal_connect (GTK_OBJECT(tree), "select_child",
		      GTK_SIGNAL_FUNC(cb_select_child), tree);
  gtk_signal_connect (GTK_OBJECT(tree), "unselect_child",
		      GTK_SIGNAL_FUNC(cb_unselect_child), tree);
  gtk_signal_connect (GTK_OBJECT(tree), "selection_changed",
		      GTK_SIGNAL_FUNC(cb_selection_changed), tree);
  /* Add it to the scrolled window */
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled_win),
                                         tree);
  /* Set the selection mode */
  gtk_tree_set_selection_mode (GTK_TREE(tree),
			       GTK_SELECTION_MULTIPLE);
  /* Show it */
  gtk_widget_show (tree);

  /* Now, create a clist and attach it to the second pane */

  clist = gtk_clist_new_with_titles(4, col_titles);

  gtk_container_add (GTK_CONTAINER(scrolled_win2), clist);

  gtk_widget_show(clist);

  /* Now, build the top level display ... */

  if ((dh = smbc_opendir("smb:///")) < 0) {

    fprintf(stderr, "Could not list default workgroup: smb:///: %s\n",
	    strerror(errno));

    exit(1);

  }

  /* Create a tree item for Whole Network */

  item = gtk_tree_item_new_with_label ("Whole Network");
  /* Connect all GtkItem:: and GtkTreeItem:: signals */
  gtk_signal_connect (GTK_OBJECT(item), "select",
		      GTK_SIGNAL_FUNC(cb_itemsignal), "select");
  gtk_signal_connect (GTK_OBJECT(item), "deselect",
		      GTK_SIGNAL_FUNC(cb_itemsignal), "deselect");
  gtk_signal_connect (GTK_OBJECT(item), "toggle",
		      GTK_SIGNAL_FUNC(cb_itemsignal), "toggle");
  gtk_signal_connect (GTK_OBJECT(item), "expand",
		      GTK_SIGNAL_FUNC(cb_wholenet), "expand");
  gtk_signal_connect (GTK_OBJECT(item), "collapse",
		      GTK_SIGNAL_FUNC(cb_wholenet), "collapse");
  /* Add it to the parent tree */
  gtk_tree_append (GTK_TREE(tree), item);
  /* Show it - this can be done at any time */
  gtk_widget_show (item);

  subtree = gtk_tree_new();  /* A subtree for Whole Network */

  gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), subtree);

  gtk_signal_connect (GTK_OBJECT(subtree), "select_child",
		      GTK_SIGNAL_FUNC(cb_select_child), tree);
  gtk_signal_connect (GTK_OBJECT(subtree), "unselect_child",
		      GTK_SIGNAL_FUNC(cb_unselect_child), tree);

  /* Now, get the items in smb:/// and add them to the tree */

  dirp = (struct smbc_dirent *)dirbuf;

  while ((err = smbc_getdents(dh, (struct smbc_dirent *)dirbuf, 
			      sizeof(dirbuf))) != 0) {

    if (err < 0) { /* Handle the error */

      fprintf(stderr, "Could not read directory for smbc:///: %s\n",
	      strerror(errno));

      exit(1);

    }

    fprintf(stdout, "Dir len: %u\n", err);

    while (err > 0) { /* Extract each entry and make a sub-tree */
      struct tree_data *my_data;
      int dirlen = dirp->dirlen;

      my_data = make_tree_data(dirp->smbc_type, dirp->name);

      item = gtk_tree_item_new_with_label(dirp->name);
      /* Connect all GtkItem:: and GtkTreeItem:: signals */
      gtk_signal_connect (GTK_OBJECT(item), "select",
			  GTK_SIGNAL_FUNC(cb_itemsignal), "select");
      gtk_signal_connect (GTK_OBJECT(item), "deselect",
			  GTK_SIGNAL_FUNC(cb_itemsignal), "deselect");
      gtk_signal_connect (GTK_OBJECT(item), "toggle",
			  GTK_SIGNAL_FUNC(cb_itemsignal), "toggle");
      gtk_signal_connect (GTK_OBJECT(item), "expand",
			  GTK_SIGNAL_FUNC(cb_itemsignal), "expand");
      gtk_signal_connect (GTK_OBJECT(item), "collapse",
			  GTK_SIGNAL_FUNC(cb_itemsignal), "collapse");
      /* Add it to the parent tree */
      gtk_tree_append (GTK_TREE(tree), item);
      /* Show it - this can be done at any time */
      gtk_widget_show (item);

      gtk_object_set_user_data(GTK_OBJECT(item), (gpointer)my_data);

      fprintf(stdout, "Added: %s, len: %u\n", dirp->name, dirlen);

      subtree = gtk_tree_new();

      gtk_tree_item_set_subtree(GTK_TREE_ITEM(item), subtree);

      gtk_signal_connect (GTK_OBJECT(subtree), "select_child",
			  GTK_SIGNAL_FUNC(cb_select_child), tree);
      gtk_signal_connect (GTK_OBJECT(subtree), "unselect_child",
			  GTK_SIGNAL_FUNC(cb_unselect_child), tree);

      (char *)dirp += dirlen;
      err -= dirlen;

    }

  }

  smbc_closedir(dh); /* FIXME, check for error :-) */

  /* Show the window and loop endlessly */
  gtk_main();
  return 0;
}
/* example-end */

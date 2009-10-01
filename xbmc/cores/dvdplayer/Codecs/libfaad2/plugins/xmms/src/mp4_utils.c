/*
 * some functions for MP4 files
*/

#include "mp4ff.h"
#include "faad.h"

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <xmms/plugin.h>
#include <xmms/titlestring.h>
#include <xmms/util.h>

const char *mp4AudioNames[]=
  {
    "MPEG-1 Audio Layers 1,2 or 3",
    "MPEG-2 low biterate (MPEG-1 extension) - MP3",
    "MPEG-2 AAC Main Profile",
    "MPEG-2 AAC Low Complexity profile",
    "MPEG-2 AAC SSR profile",
    "MPEG-4 audio (MPEG-4 AAC)",
    0
  };

/* MPEG-4 Audio types from 14496-3 Table 1.5.1 (from mp4.h)*/
const char *mpeg4AudioNames[]=
  {
    "!!!!MPEG-4 Audio track Invalid !!!!!!!",
    "MPEG-4 AAC Main profile",
    "MPEG-4 AAC Low Complexity profile",
    "MPEG-4 AAC SSR profile",
    "MPEG-4 AAC Long Term Prediction profile",
    "MPEG-4 AAC Scalable",
    "MPEG-4 CELP",
    "MPEG-4 HVXC",
    "MPEG-4 Text To Speech",
    "MPEG-4 Main Synthetic profile",
    "MPEG-4 Wavetable Synthesis profile",
    "MPEG-4 MIDI Profile",
    "MPEG-4 Algorithmic Synthesis and Audio FX profile"
  };

static GtkWidget *mp4_info_dialog = NULL;

/*
 * find AAC track
*/

int getAACTrack(mp4ff_t *infile)
{
  int i, rc;
  int numTracks = mp4ff_total_tracks(infile);

  printf("total-tracks: %d\n", numTracks);
  for(i=0; i<numTracks; i++){
    unsigned char*	buff = 0;
    int			buff_size = 0;
    mp4AudioSpecificConfig mp4ASC;

    printf("testing-track: %d\n", i);
    mp4ff_get_decoder_config(infile, i, &buff, &buff_size);
    if(buff){
      rc = NeAACDecAudioSpecificConfig(buff, buff_size, &mp4ASC);
      g_free(buff);
      if(rc < 0)
	continue;
      return(i);
    }
  }
  return(-1);
}

uint32_t read_callback(void *user_data, void *buffer, uint32_t length)
{
  return fread(buffer, 1, length, (FILE*)user_data);
}

uint32_t seek_callback(void *user_data, uint64_t position)
{
  return fseek((FILE*)user_data, position, SEEK_SET);
}

mp4ff_callback_t *getMP4FF_cb(FILE *mp4file)
{
  mp4ff_callback_t* mp4cb = malloc(sizeof(mp4ff_callback_t));
  mp4cb->read = read_callback;
  mp4cb->seek = seek_callback;
  mp4cb->user_data = mp4file;
  return mp4cb;
}

/*
 * Function to display an info box for an mp4 file.
 * This code is based heavily on fileinfo.c from the xmms mpg123
 * plugin, and the info box layout mimics that plugin.
*/
void create_mp4_info_dialog (char *filename, FILE *mp4file, mp4ff_t *infile, gint mp4track)
{
  char *window_title, *value, *value2;
  static GtkWidget *filename_entry, *title_entry, *artist_entry, *album_entry;
  static GtkWidget *genre_entry, *year_entry, *track_entry, *comment_entry;
  static GtkWidget *mp4_info_label;

  if (!mp4_info_dialog)
  {
    GtkWidget *dialog_vbox1, *vbox1, *hbox2, *hbox3, *hbox4;
    GtkWidget *frame2, *frame3, *table1, *dialog_action_area1;
    GtkWidget *filename_label, *title_label, *artist_label, *album_label;
    GtkWidget *genre_label, *year_label, *track_label, *comment_label;
    GtkWidget *close_button;

    mp4_info_dialog = gtk_dialog_new ();
    gtk_object_set_data (GTK_OBJECT (mp4_info_dialog), "mp4_info_dialog", mp4_info_dialog);
    gtk_window_set_policy (GTK_WINDOW (mp4_info_dialog), TRUE, TRUE, FALSE);
    gtk_signal_connect(GTK_OBJECT (mp4_info_dialog), "destroy",
                                   gtk_widget_destroyed, &mp4_info_dialog);

    dialog_vbox1 = GTK_DIALOG (mp4_info_dialog)->vbox;
    gtk_object_set_data (GTK_OBJECT (mp4_info_dialog), "dialog_vbox1", dialog_vbox1);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_vbox1), 3);

    hbox2 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox2);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "hbox2", hbox2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox2, FALSE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox2), 3);

    filename_label = gtk_label_new ("Filename: ");
    gtk_widget_ref (filename_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "filename_label", filename_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (hbox2), filename_label, FALSE, FALSE, 0);

    filename_entry = gtk_entry_new ();
    gtk_widget_ref (filename_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "filename_entry", filename_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (hbox2), filename_entry, TRUE, TRUE, 0);
    gtk_entry_set_editable (GTK_ENTRY (filename_entry), FALSE);

    hbox3 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox3);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "hbox3", hbox3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (dialog_vbox1), hbox3, TRUE, TRUE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox3), 3);

    frame2 = gtk_frame_new ("Tag Info: ");
    gtk_widget_ref (frame2);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "frame2", frame2,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (hbox3), frame2, TRUE, TRUE, 0);

    table1 = gtk_table_new (6, 2, FALSE);
    gtk_widget_ref (table1);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "table1", table1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add (GTK_CONTAINER (frame2), table1);
    gtk_container_set_border_width (GTK_CONTAINER (table1), 5);

    comment_label = gtk_label_new ("Comment: ");
    gtk_widget_ref (comment_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "comment_label", comment_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), comment_label, 0, 1, 5, 6,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (comment_label), 1, 0.5);

    genre_label = gtk_label_new ("Genre: ");
    gtk_widget_ref (genre_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "genre_label", genre_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), genre_label, 0, 1, 4, 5,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (genre_label), 1, 0.5);

    album_label = gtk_label_new ("Album: ");
    gtk_widget_ref (album_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "album_label", album_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), album_label, 0, 1, 2, 3,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (album_label), 1, 0.5);

    title_label = gtk_label_new ("Title: ");
    gtk_widget_ref (title_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "title_label", title_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), title_label, 0, 1, 0, 1,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (title_label), 1, 0.5);

    artist_label = gtk_label_new ("Artist: ");
    gtk_widget_ref (artist_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "artist_label", artist_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), artist_label, 0, 1, 1, 2,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (artist_label), 1, 0.5);

    year_label = gtk_label_new ("Year: ");
    gtk_widget_ref (year_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "year_label", year_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), year_label, 0, 1, 3, 4,
                      (GtkAttachOptions) (GTK_FILL),
                      (GtkAttachOptions) (GTK_EXPAND), 0, 0);
    gtk_misc_set_alignment (GTK_MISC (year_label), 1, 0.5);

    hbox4 = gtk_hbox_new (FALSE, 0);
    gtk_widget_ref (hbox4);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "hbox4", hbox4,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), hbox4, 1, 2, 3, 4,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (GTK_FILL), 0, 0);

    year_entry = gtk_entry_new ();
    gtk_widget_ref (year_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "year_entry", year_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (hbox4), year_entry, FALSE, FALSE, 0);
    gtk_widget_set_usize (year_entry, 60, -2);
    gtk_entry_set_editable (GTK_ENTRY (year_entry), FALSE);

    track_label = gtk_label_new ("     Track: ");
    gtk_widget_ref (track_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "track_label", track_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (hbox4), track_label, FALSE, FALSE, 0);
    gtk_misc_set_alignment (GTK_MISC (track_label), 1, 0.5);

    track_entry = gtk_entry_new ();
    gtk_widget_ref (track_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "track_entry", track_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (hbox4), track_entry, FALSE, FALSE, 0);
    gtk_widget_set_usize (track_entry, 60, -2);
    gtk_entry_set_editable (GTK_ENTRY (track_entry), FALSE);

    title_entry = gtk_entry_new ();
    gtk_widget_ref (title_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "title_entry", title_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), title_entry, 1, 2, 0, 1,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_editable (GTK_ENTRY (title_entry), FALSE);

    artist_entry = gtk_entry_new ();
    gtk_widget_ref (artist_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "artist_entry", artist_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), artist_entry, 1, 2, 1, 2,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_editable (GTK_ENTRY (artist_entry), FALSE);

    album_entry = gtk_entry_new ();
    gtk_widget_ref (album_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "album_entry", album_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), album_entry, 1, 2, 2, 3,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_editable (GTK_ENTRY (album_entry), FALSE);

    genre_entry = gtk_entry_new ();
    gtk_widget_ref (genre_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "genre_entry", genre_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), genre_entry, 1, 2, 4, 5,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_editable (GTK_ENTRY (genre_entry), FALSE);

    comment_entry = gtk_entry_new ();
    gtk_widget_ref (comment_entry);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "comment_entry", comment_entry,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_table_attach (GTK_TABLE (table1), comment_entry, 1, 2, 5, 6,
                      (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                      (GtkAttachOptions) (0), 0, 0);
    gtk_entry_set_editable (GTK_ENTRY (comment_entry), FALSE);

    frame3 = gtk_frame_new ("MP4 Info: ");
    gtk_widget_ref (frame3);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "frame3", frame3,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (hbox3), frame3, FALSE, TRUE, 0);

    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_ref (vbox1);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "vbox1", vbox1,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_container_add (GTK_CONTAINER (frame3), vbox1);
    gtk_container_set_border_width (GTK_CONTAINER (vbox1), 5);

    mp4_info_label = gtk_label_new ("");
    gtk_widget_ref (mp4_info_label);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "mp4_info_label", mp4_info_label,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (vbox1), mp4_info_label, TRUE, TRUE, 0);
    gtk_label_set_justify (GTK_LABEL (mp4_info_label), GTK_JUSTIFY_LEFT);
    gtk_misc_set_alignment (GTK_MISC (mp4_info_label), 0, 0);

    dialog_action_area1 = GTK_DIALOG (mp4_info_dialog)->action_area;
    gtk_object_set_data (GTK_OBJECT (mp4_info_dialog), "dialog_action_area1", dialog_action_area1);
    gtk_container_set_border_width (GTK_CONTAINER (dialog_action_area1), 4);

    close_button = gtk_button_new_with_label ("Close");
    gtk_widget_ref (close_button);
    gtk_object_set_data_full (GTK_OBJECT (mp4_info_dialog), "close_button", close_button,
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_box_pack_start (GTK_BOX (dialog_action_area1), close_button, FALSE, FALSE, 0);

    gtk_signal_connect_object (GTK_OBJECT (close_button), "clicked",
                               GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (mp4_info_dialog));

  }

  window_title = g_strdup_printf("File Info - %s", g_basename(filename));
  gtk_window_set_title (GTK_WINDOW (mp4_info_dialog), window_title);
  g_free(window_title);

  gtk_entry_set_text (GTK_ENTRY (filename_entry), filename);

  gtk_entry_set_text (GTK_ENTRY (title_entry), "");
  gtk_entry_set_text (GTK_ENTRY (artist_entry), "");
  gtk_entry_set_text (GTK_ENTRY (album_entry), "");
  gtk_entry_set_text (GTK_ENTRY (year_entry), "");
  gtk_entry_set_text (GTK_ENTRY (track_entry), "");
  gtk_entry_set_text (GTK_ENTRY (genre_entry), "");
  gtk_entry_set_text (GTK_ENTRY (comment_entry), "");

  if ((mp4ff_meta_get_title(infile, &value)) && value != NULL) {
    gtk_entry_set_text (GTK_ENTRY(title_entry), value);
    g_free(value);
  }
  if ((mp4ff_meta_get_artist(infile, &value)) && value != NULL) {
    gtk_entry_set_text (GTK_ENTRY(artist_entry), value);
    g_free(value);
  }
  if ((mp4ff_meta_get_album(infile, &value)) && value != NULL) {
    gtk_entry_set_text (GTK_ENTRY(album_entry), value);
    g_free(value);
  }
  if ((mp4ff_meta_get_date(infile, &value)) && value != NULL) {
    gtk_entry_set_text (GTK_ENTRY(year_entry), value);
    g_free(value);
  }
  if ((mp4ff_meta_get_track(infile, &value)) && value != NULL) {
    if ((mp4ff_meta_get_totaltracks(infile, &value2)) && value2 != NULL) {
      char *tmp = g_strdup_printf("%s of %s", value, value2);
      g_free(value2);
      g_free(value);
      value = tmp;
    }
    gtk_entry_set_text (GTK_ENTRY(track_entry), value);
    g_free(value);
  }
  if ((mp4ff_meta_get_genre(infile, &value)) && value != NULL) {
    gtk_entry_set_text (GTK_ENTRY(genre_entry), value);
    g_free(value);
  }
  if ((mp4ff_meta_get_comment(infile, &value)) && value != NULL) {
    gtk_entry_set_text (GTK_ENTRY(comment_entry), value);
    g_free(value);
  }

  // Get the length of the track.
  double track_duration = mp4ff_get_track_duration(infile, mp4track);
  unsigned long time_scale = mp4ff_time_scale(infile, mp4track);
  unsigned long length = (track_duration / time_scale);
  int min = length / 60;
  int sec = length % 60;

  // Get other info about the track.
  unsigned long bitrate = mp4ff_get_avg_bitrate(infile, mp4track) / 1000;
  unsigned long samplerate = mp4ff_get_sample_rate(infile, mp4track);
  unsigned long channels = mp4ff_get_channel_count(infile, mp4track);
  unsigned long audio_type = mp4ff_get_audio_type(infile, mp4track);
  fseek(mp4file, 0, SEEK_END);
  int filesize = ftell(mp4file) / 1024;

  value = g_strdup_printf("Length: %d:%d\nAvg. Bitrate: %ld kbps\nSample Rate: %ld Hz\nChannels: %ld\nAudio Type: %ld\nFile Size: %d KB", min, sec, bitrate, samplerate, channels, audio_type, filesize);
  gtk_label_set_text (GTK_LABEL(mp4_info_label), value);
  g_free(value);

  gtk_widget_show_all(mp4_info_dialog);
}


void getMP4info(char* filename, FILE* mp4file)
{
  mp4ff_callback_t*	mp4cb;
  mp4ff_t*		infile;
  gint			mp4track;

  mp4cb = getMP4FF_cb(mp4file);
  if ((infile = mp4ff_open_read_metaonly(mp4cb)) &&
      ((mp4track = getAACTrack(infile)) >= 0)){
    create_mp4_info_dialog (filename, mp4file, infile, mp4track);
  }
  if(infile) mp4ff_close(infile);
  if(mp4cb) g_free(mp4cb);
}

/* Get the xmms titlestring for the file based on metadata.
The following code was adapted from the gtkpod project, specifically
mp4file.c (C) Jorg Schuler, but written to use the mp4ff library.  The
mpg123 input plugin for xmms was used as a guide for this function.
	--Jason Arroyo, 2004 */
char *getMP4title(mp4ff_t *infile, char *filename) {
	char *ret=NULL;
	gchar *value, *path, *temp;

	TitleInput *input;
	XMMS_NEW_TITLEINPUT(input);

	// Fill in the TitleInput with the relevant data
	// from the mp4 file that can be used to display the title.
	mp4ff_meta_get_title(infile, &input->track_name);
        mp4ff_meta_get_artist(infile, &input->performer);
	mp4ff_meta_get_album(infile, &input->album_name);
	if (mp4ff_meta_get_track(infile, &value) && value != NULL) {
		input->track_number = atoi(value);
		g_free(value);
	}
	if (mp4ff_meta_get_date(infile, &value) && value != NULL) {
		input->year = atoi(value);
		g_free(value);
	}
	mp4ff_meta_get_genre(infile, &input->genre);
	mp4ff_meta_get_comment(infile, &input->comment);
	input->file_name = g_strdup(g_basename(filename));
	path = g_strdup(filename);
	temp = strrchr(path, '.');
	if (temp != NULL) {++temp;}
	input->file_ext = g_strdup_printf("%s", temp);
	temp = strrchr(path, '/');
	if (temp) {*temp = '\0';}
	input->file_path = g_strdup_printf("%s/", path);

	// Use the default xmms title format to format the
	// title from the above info.
	ret = xmms_get_titlestring(xmms_get_gentitle_format(), input);

        g_free(input->track_name);
        g_free(input->performer);
        g_free(input->album_name);
        g_free(input->genre);
        g_free(input->comment);
        g_free(input->file_name);
        g_free(input->file_ext);
        g_free(input->file_path);
	g_free(input);
	g_free(path);

	return ret;
}

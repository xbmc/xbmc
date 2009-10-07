enum addon_log {
  REFMEM_DEBUG,
  REFMEM_INFO,
  REFMEM_NOTICE,
  REFMEM_ERROR
};

/* An individual preset */
struct viz_preset {
  char *name;
};

/* A list of presets */
struct viz_preset_list {
  viz_preset_t *list;
  long count;
};

/* An individual track */
struct viz_track {
  char *title;
  char *artist;
  char *album;
  char *albumartist;
  char *genre;
  char *comment;
  char *lyrics;
  int  tracknum;
  int  discnum;
  int  duration;
  int  year;
  char rating;
};

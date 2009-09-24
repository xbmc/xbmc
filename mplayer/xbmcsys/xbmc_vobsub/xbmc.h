#ifndef __XBMC_H
#define __XBMC_H 1

#define MAX_XBMC_NAME 10
#define MAX_XBMC_SUBTITLES 32

#define XBMC_SUBTYPE_STANDARD 1
#define XBMC_SUBTYPE_VOBSUB 2
#define XBMC_SUBTYPE_DVDSUB 3
#define XBMC_SUBTYPE_OGGSUB 4
#define XBMC_SUBTYPE_MKVSUB 5

typedef struct 
{
	int id; //The internal id of a subtitle of a certain type.
	char name[MAX_XBMC_NAME]; //Name as given by the file playing
  char* desc; //Possibly some descriptive data, not used at the moment.
	int type; //Subtitle type
	int invalid; //Will be set if this subtitle has been invalidated for some reason. can't be used
} xbmc_subtitle;

int xbmc_cancel;

int xbmc_mkv_audiocount(void *mkvdemuxer);
int xbmc_mkv_fill_audioinfo(void *mkvdemuxer, void* si, int aid);
int xbmc_mkv_get_aid_from_num(void *mkvdemuxer, int num);

void get_lang_ext(char *filename, char *lang);
#endif

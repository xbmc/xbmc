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

xbmc_subtitle xbmc_subtitles[MAX_XBMC_SUBTITLES];
int xbmc_sub_count;
int xbmc_sub_current;


int xbmc_cancel;

inline int xbmc_sid_from_num(int num);
inline int xbmc_num_from_sid(int sid, int type);
void xbmc_addsub(int id, char* name, int type, int invalid);
void xbmc_mkv_updatesubs(void *mkvdemuxer);
int xbmc_mkv_audiocount(void *mkvdemuxer);
int xbmc_mkv_fill_audioinfo(void *mkvdemuxer, void* si, int aid);
int xbmc_mkv_get_aid_from_num(void *mkvdemuxer, int num);
#endif

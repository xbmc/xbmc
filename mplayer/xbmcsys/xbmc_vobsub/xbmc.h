#ifndef __XBMC_H
#define __XBMC_H 1


#define MAX_XBMC_NAME 10

#define XBMC_SUBTYPE_STANDARD 1
#define XBMC_SUBTYPE_VOBSUB 2
#define XBMC_SUBTYPE_DVDSUB 3
#define XBMC_SUBTYPE_OGGSUB 4
#define XBMC_SUBTYPE_MKVSUB 5

typedef struct 
{
	int id;
	char name[MAX_XBMC_NAME];
	int type;
	int invalid;
} xbmc_subtitle;

xbmc_subtitle xbmc_subtitles[32];
int xbmc_sub_count;
int xbmc_sub_current;


int xbmc_cancel;

inline int xbmc_sid_from_num(int num);
inline int xbmc_num_from_sid(int sid, int type);
void xbmc_addsub(int id, char* name, int type, int invalid);
void xbmc_update_matroskasubs(void *mkvdemuxer);

#endif

#include "xbmc.h"
#include "string.h"

inline int xbmc_sid_from_num(int num)
{
	if(num<xbmc_sub_count)
		return xbmc_subtitles[num].id;
	else
		return -1;
}

inline int xbmc_num_from_sid(int sid, int type)
{
	int i;
	for(i=0;i<xbmc_sub_count;i++)
	{
		if(xbmc_subtitles[i].id==sid && xbmc_subtitles[i].type==type)
			return i;
	}
	return -1;
}

void xbmc_addsub(int id, char* name, int type,int invalid)
{
	xbmc_subtitles[xbmc_sub_count].id = id;
	if(name)
		strncpy(xbmc_subtitles[xbmc_sub_count].name,name, MAX_XBMC_NAME);
	else
		xbmc_subtitles[xbmc_sub_count].name[0] = 0;
	xbmc_subtitles[xbmc_sub_count].type = type;
	xbmc_subtitles[xbmc_sub_count].invalid = invalid;
	xbmc_sub_count++;
}

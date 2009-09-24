#include "xbmc.h"
#include "string.h"

void get_lang_ext(char *filename, char *lang)
{
	int i,l;
	int p1 = 0;
	int p2 = 0;
	char ch;
	
	strcpy(lang,"");
	l = strlen(filename);
	for(i=0; i<l; i++)
	{
		ch = filename[i];
		if (ch == '.')
		{
			p2 = p1;
			p1 = i;		
		}
	}

	if (p2)
		p2++;	
	l = p1 - p2;
	if ((p1 > 0) && (p2 > 0) && (l < MAX_XBMC_NAME))
	{
		strncpy(lang,filename+p2,l);
		lang[l] = 0;
	}	
}

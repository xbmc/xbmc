#include <ogg/os_types.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

char *strdup(const char *inStr)
{
	char *outStr = NULL;
	
	if (inStr == NULL) {
		return NULL;
	}
	
	outStr = _ogg_malloc(strlen(inStr) + 1);
	
	if (outStr != NULL) {
		strcpy(outStr, inStr);
	}
	
	return outStr;
}

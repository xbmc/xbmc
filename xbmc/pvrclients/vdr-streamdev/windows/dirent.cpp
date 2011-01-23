//#include <malloc.h>

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <direct.h>
#include <string.h>
#include <errno.h>

#include "pvrclient-vdr_os.h"
#include <dirent.h>

/**********************************************************************
 * Implement dirent-style opendir/readdir/rewinddir/closedir on Win32
 *
 * Functions defined are opendir(), readdir(), rewinddir() and
 * closedir() with the same prototypes as the normal dirent.h
 * implementation.
 *
 * Does not implement telldir(), seekdir(), or scandir().  The dirent
 * struct is compatible with Unix, except that d_ino is always 1 and
 * d_off is made up as we go along.
 *
 * The DIR typedef is not compatible with Unix.
 **********************************************************************/

DIR *opendir(const char *dir)
{
	DIR		*dirp;
	char	*filespec;
	long	handle;
	int		index;

	filespec = (char *)malloc(strlen(dir) + 2 + 1);
	strcpy(filespec, dir);
	index = (int)strlen(filespec) - 1;
	if (index >= 0 && (filespec[index] == '/' || 
	   (filespec[index] == '\\' && !IsDBCSLeadByte(filespec[index-1]))))
		filespec[index] = '\0';
	strcat(filespec, "/*");

	dirp = (DIR *) malloc(sizeof(DIR));
	dirp->offset = 0;
	dirp->finished = 0;

	if ((handle = _findfirst(filespec, &(dirp->fileinfo))) < 0) 
	{
		if (errno == ENOENT || errno == EINVAL) 
			dirp->finished = 1;
		else 
		{
			free(dirp);
			free(filespec);
			return NULL;
		}
	}
	dirp->dirname = strdup(dir);
	dirp->handle = handle;
	free(filespec);

	return dirp;
}

int closedir(DIR *dp)
{
	int iret = -1;
	if (!dp)
		return iret;
	iret = _findclose(dp->handle);
	if (iret == 0 && dp->dirname)
		free(dp->dirname);
	if (iret == 0 && dp)
		free(dp);

	return iret;
}

struct dirent *readdir(DIR *dp)
{
	if (!dp || dp->finished)
		return NULL;

	if (dp->offset != 0) 
	{
		if (_findnext(dp->handle, &(dp->fileinfo)) < 0) 
		{
			dp->finished = 1;
			return NULL;
		}
	}
	dp->offset++;

	strcpy(dp->dent.d_name, dp->fileinfo.name);/*, _MAX_FNAME+1);*/
	dp->dent.d_ino = 1;
	dp->dent.d_reclen = (unsigned short)strlen(dp->dent.d_name);
	dp->dent.d_off = dp->offset;

	return &(dp->dent);
}

int readdir_r(DIR *dp, struct dirent *entry, struct dirent **result)
{
	if (!dp || dp->finished) 
	{
		*result = NULL;
		return -1;
	}

	if (dp->offset != 0) 
	{
		if (_findnext(dp->handle, &(dp->fileinfo)) < 0) 
		{
			dp->finished = 1;
			*result = NULL;
			return -1;
		}
	}
	dp->offset++;

	strcpy(dp->dent.d_name, dp->fileinfo.name);/*, _MAX_FNAME+1);*/
	dp->dent.d_ino = 1;
	dp->dent.d_reclen = (unsigned short)strlen(dp->dent.d_name);
	dp->dent.d_off = dp->offset;

	memcpy(entry, &dp->dent, sizeof(*entry));

	*result = &dp->dent;

	return 0;
}

int rewinddir(DIR *dp)
{
	char	*filespec;
	long	handle;
	int		index;

	_findclose(dp->handle);

	dp->offset = 0;
	dp->finished = 0;

	filespec = (char *)malloc(strlen(dp->dirname) + 2 + 1);
	strcpy(filespec, dp->dirname);
	index = (int)(strlen(filespec) - 1);
	if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
		filespec[index] = '\0';
	strcat(filespec, "/*");

	if ((handle = _findfirst(filespec, &(dp->fileinfo))) < 0) 
	{
		if (errno == ENOENT || errno == EINVAL)
			dp->finished = 1;
	}
	dp->handle = handle;
	free(filespec);

	return 0;
}

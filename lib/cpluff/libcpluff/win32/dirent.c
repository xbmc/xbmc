/*

    Implementation of POSIX directory browsing functions and types for Win32.

    Author:  Kevlin Henney (kevlin@acm.org, kevlin@curbralan.com)
    History: Created March 1997. Updated June 2003.
    Rights:  See end of file.

*/

#include "dirent.h"
#include <errno.h>
#include <io.h> /* _findfirst and _findnext set errno iff they return -1 */
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct DIR
{
    long                handle; /* -1 for failed rewind */
    struct _finddata_t  info;
    struct dirent       result; /* d_name null iff first time */
    char                *name;  /* null-terminated char string */
};

DIR *opendir(const char *name)
{
    DIR *dir = 0;

    if(name && name[0])
    {
        size_t base_length = strlen(name);
        const char *all = /* search pattern must end with suitable wildcard */
            strchr("/\\", name[base_length - 1]) ? "*" : "/*";

        if((dir = (DIR *) malloc(sizeof *dir)) != 0 &&
           (dir->name = (char *) malloc(base_length + strlen(all) + 1)) != 0)
        {
            strcat(strcpy(dir->name, name), all);

            if((dir->handle = _findfirst(dir->name, &dir->info)) != -1)
            {
                dir->result.d_name = 0;
            }
            else /* rollback */
            {
                free(dir->name);
                free(dir);
                dir = 0;
            }
        }
        else /* rollback */
        {
            free(dir);
            dir   = 0;
            errno = ENOMEM;
        }
    }
    else
    {
        errno = EINVAL;
    }

    return dir;
}

int closedir(DIR *dir)
{
    int result = -1;

    if(dir)
    {
        if(dir->handle != -1)
        {
            result = _findclose(dir->handle);
        }

        free(dir->name);
        free(dir);
    }

    if(result == -1) /* map all errors to EBADF */
    {
        errno = EBADF;
    }

    return result;
}

struct dirent *readdir(DIR *dir)
{
    struct dirent *result = 0;

    if(dir && dir->handle != -1)
    {
        if(!dir->result.d_name || _findnext(dir->handle, &dir->info) != -1)
        {
            result         = &dir->result;
            result->d_name = dir->info.name;
        }
    }
    else
    {
        errno = EBADF;
    }

    return result;
}

void rewinddir(DIR *dir)
{
    if(dir && dir->handle != -1)
    {
        _findclose(dir->handle);
        dir->handle = _findfirst(dir->name, &dir->info);
        dir->result.d_name = 0;
    }
    else
    {
        errno = EBADF;
    }
}

// helper for scandir below
static void scandir_free_dir_entries(struct dirent*** namelist, int entries) {
	int i;
	if (!*namelist) return;
	for (i = 0; i < entries; ++i) {
		free((*namelist)[i]);
	}
	free(*namelist);
	*namelist = 0;
}

// returns the number of directory entries select or -1 if an error occurs
int scandir(
	const char* dir,
	struct dirent*** namelist,
	int(*filter)(const struct dirent*),
	int(*compar)(const void*, const void*)
) {
	int entries = 0;
	int max_entries = 1024; // assume 2*512 = 1024 entries (used for allocation)
	DIR* d;

	*namelist = 0;

	// open directory
	d = opendir(dir);
	if (!d) return -1;

	// iterate
	while (1) {
		struct dirent* ent = readdir(d);
		if (!ent) break;

		// add if no filter or filter returns non-zero
		if (filter && (0 == filter(ent))) continue;

		// resize our buffer if there is not enough room
		if (!*namelist || entries >= max_entries) {
			struct dirent** new_entries;

			max_entries *= 2;
			new_entries = (struct dirent **)realloc(*namelist, max_entries);
			if (!new_entries) {
				scandir_free_dir_entries(namelist, entries);
				closedir(d);
				errno = ENOMEM;
				return -1;
			}

			*namelist = new_entries;
		}

		// allocate new entry
		(*namelist)[entries] = (struct dirent *)malloc(sizeof(struct dirent) + strlen(ent->d_name) + 1);
		if (!(*namelist)[entries]) {
			scandir_free_dir_entries(namelist, entries);
			closedir(d);
			errno = ENOMEM;
			return -1;
		}

		// copy entry info
		*(*namelist)[entries] = *ent;

		// and then we tack the string onto the end
		{
			char* dest = (char*)((*namelist)[entries]) + sizeof(struct dirent);
			strcpy(dest, ent->d_name);
			(*namelist)[entries]->d_name = dest;
		}

		++entries;
	}

	closedir(d);

	// sort
	if (*namelist && compar) qsort(*namelist, entries, sizeof((*namelist)[0]), compar);

	return entries;
}

int alphasort(const void* lhs, const void* rhs) {
	const struct dirent* lhs_ent = *(struct dirent**)lhs;
	const struct dirent* rhs_ent = *(struct dirent**)rhs;
	return _strcmpi(lhs_ent->d_name, rhs_ent->d_name);
}

#ifdef __cplusplus
}
#endif

/*

    Copyright Kevlin Henney, 1997, 2003. All rights reserved.

    Permission to use, copy, modify, and distribute this software and its
    documentation for any purpose is hereby granted without fee, provided
    that this copyright and permissions notice appear in all copies and
    derivatives.

    This software is supplied "as is" without express or implied warranty.

    But that said, if there are any problems please get in touch.

*/

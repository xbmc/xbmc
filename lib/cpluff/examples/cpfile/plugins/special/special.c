/*
 * Copyright 2007 Johannes Lehtinen
 * This file is free software; Johannes Lehtinen gives unlimited
 * permission to copy, distribute and modify it.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cpluff.h>
#include <core.h>

#if defined(HAVE_LSTAT)
#define STAT lstat
#elif defined(HAVE_STAT)
#define STAT stat
#endif


/* ------------------------------------------------------------------------
 * Internal functions
 * ----------------------------------------------------------------------*/

/**
 * Classifies a file by using stat(2). This classifier does not need
 * any classifier data so we use NULL as dummy data pointer. Therefore
 * we do not need a plug-in instance either as there is no data to be
 * initialized.
 */
static int classify(void *dummy, const char *path) {
#ifdef STAT
	struct stat s;
	const char *type;
	
	// Stat the file
	if (STAT(path, &s)) {
		fflush(stdout);
		perror("stat failed");
		
		// No point for other classifiers to classify this
		return 1;
	}
	
	// Check if this is a special file
	if ((s.st_mode & S_IFMT) == S_IFDIR) {
		type = "directory";
#ifdef S_IFCHR
	} else if ((s.st_mode & S_IFMT) == S_IFCHR) {
		type = "character device";
#endif
#ifdef S_IFBLK
	} else if ((s.st_mode & S_IFMT) == S_IFBLK) {
		type = "block device";
#endif
#ifdef S_IFLNK
	} else if ((s.st_mode & S_IFMT) == S_IFLNK) {
		type = "symbolic link";
#endif
	} else {
		
		// Did not recognize it, let other plug-ins try
		return 0;
	}
		
	// Print recognized file type
	fputs(type, stdout);
	putchar('\n');
	return 1;
#else
	return 0;
#endif
}


/* ------------------------------------------------------------------------
 * Exported classifier information
 * ----------------------------------------------------------------------*/

CP_EXPORT classifier_t cp_ex_cpfile_special_classifier = { NULL, classify };

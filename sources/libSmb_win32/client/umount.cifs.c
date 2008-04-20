/* 
   Unmount utility program for Linux CIFS VFS (virtual filesystem) client
   Copyright (C) 2005 Steve French  (sfrench@us.ibm.com)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <mntent.h>

#define UNMOUNT_CIFS_VERSION_MAJOR "0"
#define UNMOUNT_CIFS_VERSION_MINOR "5"

#ifndef UNMOUNT_CIFS_VENDOR_SUFFIX
 #ifdef _SAMBA_BUILD_
  #include "include/version.h"
  #ifdef SAMBA_VERSION_VENDOR_SUFFIX
   #define UNMOUNT_CIFS_VENDOR_SUFFIX "-"SAMBA_VERSION_OFFICIAL_STRING"-"SAMBA_VERSION_VENDOR_SUFFIX
  #else
   #define UNMOUNT_CIFS_VENDOR_SUFFIX "-"SAMBA_VERSION_OFFICIAL_STRING
  #endif /* SAMBA_VERSION_OFFICIAL_STRING and SAMBA_VERSION_VENDOR_SUFFIX */
 #else
  #define UNMOUNT_CIFS_VENDOR_SUFFIX ""
 #endif /* _SAMBA_BUILD_ */
#endif /* UNMOUNT_CIFS_VENDOR_SUFFIX */

#ifndef MNT_DETACH
#define MNT_DETACH 0x02
#endif

#ifndef MNT_EXPIRE
#define MNT_EXPIRE 0x04
#endif

#ifndef MOUNTED_LOCK
#define MOUNTED_LOCK    "/etc/mtab~"
#endif
#ifndef MOUNTED_TEMP
#define MOUNTED_TEMP    "/etc/mtab.tmp"
#endif

#define CIFS_IOC_CHECKUMOUNT _IO(0xCF, 2)
#define CIFS_MAGIC_NUMBER 0xFF534D42   /* the first four bytes of SMB PDU */
   
static struct option longopts[] = {
	{ "all", 0, NULL, 'a' },
	{ "help",0, NULL, 'h' },
	{ "read-only", 0, NULL, 'r' },
	{ "ro", 0, NULL, 'r' },
	{ "verbose", 0, NULL, 'v' },
	{ "version", 0, NULL, 'V' },
	{ "expire", 0, NULL, 'e' },
	{ "force", 0, 0, 'f' },
	{ "lazy", 0, 0, 'l' },
	{ "no-mtab", 0, 0, 'n' },
	{ NULL, 0, NULL, 0 }
};

const char * thisprogram;
int verboseflg = 0;

static void umount_cifs_usage(void)
{
	printf("\nUsage:  %s <remotetarget> <dir>\n", thisprogram);
	printf("\nUnmount the specified directory\n");
	printf("\nLess commonly used options:");
	printf("\n\t-r\tIf mount fails, retry with readonly remount.");
	printf("\n\t-n\tDo not write to mtab.");
	printf("\n\t-f\tAttempt a forced unmount, even if the fs is busy.");
	printf("\n\t-l\tAttempt lazy unmount, Unmount now, cleanup later.");
	printf("\n\t-v\tEnable verbose mode (may be useful for debugging).");
	printf("\n\t-h\tDisplay this help.");
	printf("\n\nOptions are described in more detail in the manual page");
	printf("\n\tman 8 umount.cifs\n");
	printf("\nTo display the version number of the cifs umount utility:");
	printf("\n\t%s -V\n",thisprogram);
	printf("\nInvoking the umount utility on cifs mounts, can execute");
	printf(" /sbin/umount.cifs (if present and umount -i is not specified.\n");
}

static int umount_check_perm(char * dir)
{
	int fileid;
	int rc;

	/* allow root to unmount, no matter what */
	if(getuid() == 0)
		return 0;

	/* presumably can not chdir into the target as we do on mount */
	fileid = open(dir, O_RDONLY | O_DIRECTORY | O_NOFOLLOW, 0);
	if(fileid == -1) {
		if(verboseflg)
			printf("error opening mountpoint %d %s",errno,strerror(errno));
		return errno;
	}

	rc = ioctl(fileid, CIFS_IOC_CHECKUMOUNT, NULL);

	if(verboseflg)
		printf("ioctl returned %d with errno %d %s\n",rc,errno,strerror(errno));

	if(rc == ENOTTY) {
		printf("user unmounting via %s is an optional feature of",thisprogram);
		printf(" the cifs filesystem driver (cifs.ko)");
		printf("\n\tand requires cifs.ko version 1.32 or later\n");
	} else if (rc > 0)
		printf("user unmount of %s failed with %d %s\n",dir,errno,strerror(errno));
	close(fileid);

	return rc;
}

int lock_mtab(void)
{
	int rc;
	
	rc = mknod(MOUNTED_LOCK , 0600, 0);
	if(rc == -1)
		printf("\ngetting lock file %s failed with %s\n",MOUNTED_LOCK,
				strerror(errno));
		
	return rc;	
	
}

void unlock_mtab(void)
{
	unlink(MOUNTED_LOCK);	
}

int remove_from_mtab(char * mountpoint)
{
	int rc;
	int num_matches;
	FILE * org_fd;
	FILE * new_fd;
	struct mntent * mount_entry;

	/* Do we need to check if it is a symlink to e.g. /proc/mounts
	in which case we probably do not want to update it? */

	/* Do we first need to check if it is writable? */ 

	if (lock_mtab()) {
		printf("Mount table locked\n");
		return -EACCES;
	}
	
	if(verboseflg)
		printf("attempting to remove from mtab\n");

	org_fd = setmntent(MOUNTED, "r");

	if(org_fd == NULL) {
		printf("Can not open %s\n",MOUNTED);
		unlock_mtab();
		return -EIO;
	}

	new_fd = setmntent(MOUNTED_TEMP,"w");
	if(new_fd == NULL) {
		printf("Can not open temp file %s", MOUNTED_TEMP);
		endmntent(org_fd);
		unlock_mtab();
		return -EIO;
	}

	/* BB fix so we only remove the last entry that matches BB */
	num_matches = 0;
	while((mount_entry = getmntent(org_fd)) != NULL) {
		if(strcmp(mount_entry->mnt_dir, mountpoint) == 0) {
			num_matches++;
		}
	}	
	if(verboseflg)
		printf("%d matching entries in mount table\n", num_matches);
		
	/* Is there a better way to seek back to the first entry in mtab? */
	endmntent(org_fd);
	org_fd = setmntent(MOUNTED, "r");

	if(org_fd == NULL) {
		printf("Can not open %s\n",MOUNTED);
		unlock_mtab();
		return -EIO;
	}
	
	while((mount_entry = getmntent(org_fd)) != NULL) {
		if(strcmp(mount_entry->mnt_dir, mountpoint) != 0) {
			addmntent(new_fd, mount_entry);
		} else {
			if(num_matches != 1) {
				addmntent(new_fd, mount_entry);
				num_matches--;
			} else if(verboseflg)
				printf("entry not copied (ie entry is removed)\n");
		}
	}

	if(verboseflg)
		printf("done updating tmp file\n");
	rc = fchmod (fileno (new_fd), S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	if(rc < 0) {
		printf("error %s changing mode of %s\n", strerror(errno),
			MOUNTED_TEMP);
	}
	endmntent(new_fd);

	rc = rename(MOUNTED_TEMP, MOUNTED);

	if(rc < 0) {
		printf("failure %s renaming %s to %s\n",strerror(errno),
			MOUNTED_TEMP, MOUNTED);
		unlock_mtab();
		return -EIO;
	}

	unlock_mtab();
	
	return rc;
}

int main(int argc, char ** argv)
{
	int c;
	int rc;
	int flags = 0;
	int nomtab = 0;
	int retry_remount = 0;
	struct statfs statbuf;
	char * mountpoint;

	if(argc && argv) {
		thisprogram = argv[0];
	} else {
		umount_cifs_usage();
		return -EINVAL;
	}

	if(argc < 2) {
		umount_cifs_usage();
		return -EINVAL;
	}

	if(thisprogram == NULL)
		thisprogram = "umount.cifs";

	/* add sharename in opts string as unc= parm */

	while ((c = getopt_long (argc, argv, "afhilnrvV",
			 longopts, NULL)) != -1) {
		switch (c) {
/* No code to do the following  option yet */
/*		case 'a':	       
			++umount_all;
			break; */
		case '?':
		case 'h':   /* help */
			umount_cifs_usage();
			exit(1);
		case 'n':
			++nomtab;
			break;
		case 'f':
			flags |= MNT_FORCE;
			break;
		case 'l':
			flags |= MNT_DETACH; /* lazy unmount */
			break;
		case 'e':
			flags |= MNT_EXPIRE; /* gradually timeout */
			break;
		case 'r':
			++retry_remount;
			break;
		case 'v':
			++verboseflg;
			break;
		case 'V':	   
			printf ("umount.cifs version: %s.%s%s\n",
				UNMOUNT_CIFS_VERSION_MAJOR,
				UNMOUNT_CIFS_VERSION_MINOR,
				UNMOUNT_CIFS_VENDOR_SUFFIX);
			exit (0);
		default:
			printf("unknown unmount option %c\n",c);
			umount_cifs_usage();
			exit(1);
		}
	}

	/* move past the umount options */
	argv += optind;
	argc -= optind;

	mountpoint = argv[0];

	if((argc < 1) || (argv[0] == NULL)) {
		printf("\nMissing name of unmount directory\n");
		umount_cifs_usage();
		return -EINVAL;
	}

	if(verboseflg)
		printf("optind %d unmount dir %s\n",optind, mountpoint);

	/* check if running effectively root */
	if(geteuid() != 0) {
		printf("Trying to unmount when %s not installed suid\n",thisprogram);
		if(verboseflg)
			printf("euid = %d\n",geteuid());
		return -EACCES;
	}

	/* fixup path if needed */

	/* make sure that this is a cifs filesystem */
	rc = statfs(mountpoint, &statbuf);
	
	if(rc || (statbuf.f_type != CIFS_MAGIC_NUMBER)) {
		printf("This utility only unmounts cifs filesystems.\n");
		return -EINVAL;
	}

	/* check if our uid was the one who mounted */
	rc = umount_check_perm(mountpoint);
	if (rc) {
		printf("Not permitted to unmount\n");
		return rc;
	}

	if(umount2(mountpoint, flags)) {
	/* remember to kill daemon on error */
		switch (errno) {
		case 0:
			printf("unmount failed but no error number set\n");
			break;
		default:
			printf("unmount error %d = %s\n",errno,strerror(errno));
		}
		printf("Refer to the umount.cifs(8) manual page (man 8 umount.cifs)\n");
		return -1;
	} else {
		if(verboseflg)
			printf("umount2 succeeded\n");
		if(nomtab == 0)
			remove_from_mtab(mountpoint);
	}

	return 0;
}


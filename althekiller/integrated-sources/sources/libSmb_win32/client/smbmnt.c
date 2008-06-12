/*
 *  smbmnt.c
 *
 *  Copyright (C) 1995-1998 by Paal-Kr. Engstad and Volker Lendecke
 *  extensively modified by Tridge
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#define SMBMOUNT_MALLOC 1

#include "includes.h"

#include <mntent.h>
#include <sys/utsname.h>

#include <asm/types.h>
#include <asm/posix_types.h>
#include <linux/smb.h>
#include <linux/smb_mount.h>
#include <asm/unistd.h>

#ifndef	MS_MGC_VAL
/* This may look strange but MS_MGC_VAL is what we are looking for and
	is what we need from <linux/fs.h> under libc systems and is
	provided in standard includes on glibc systems.  So...  We
	switch on what we need...  */
#include <linux/fs.h>
#endif

static uid_t mount_uid;
static gid_t mount_gid;
static int mount_ro;
static unsigned mount_fmask;
static unsigned mount_dmask;
static int user_mount;
static char *options;

static void
help(void)
{
        printf("\n");
        printf("Usage: smbmnt mount-point [options]\n");
	printf("Version %s\n\n",SAMBA_VERSION_STRING);
        printf("-s share       share name on server\n"
               "-r             mount read-only\n"
               "-u uid         mount as uid\n"
               "-g gid         mount as gid\n"
               "-f mask        permission mask for files\n"
               "-d mask        permission mask for directories\n"
               "-o options     name=value, list of options\n"
               "-h             print this help text\n");
}

static int
parse_args(int argc, char *argv[], struct smb_mount_data *data, char **share)
{
        int opt;

        while ((opt = getopt (argc, argv, "s:u:g:rf:d:o:")) != EOF)
	{
                switch (opt)
		{
                case 's':
                        *share = optarg;
                        break;
                case 'u':
			if (!user_mount) {
				mount_uid = strtol(optarg, NULL, 0);
			}
                        break;
                case 'g':
			if (!user_mount) {
				mount_gid = strtol(optarg, NULL, 0);
			}
                        break;
                case 'r':
                        mount_ro = 1;
                        break;
                case 'f':
                        mount_fmask = strtol(optarg, NULL, 8);
                        break;
                case 'd':
                        mount_dmask = strtol(optarg, NULL, 8);
                        break;
		case 'o':
			options = optarg;
			break;
                default:
                        return -1;
                }
        }
        return 0;
        
}

static char *
fullpath(const char *p)
{
        char path[PATH_MAX+1];

	if (strlen(p) > PATH_MAX) {
		return NULL;
	}

        if (realpath(p, path) == NULL) {
		fprintf(stderr,"Failed to find real path for mount point %s: %s\n",
			p, strerror(errno));
		exit(1);
	}
	return strdup(path);
}

/* Check whether user is allowed to mount on the specified mount point. If it's
   OK then we change into that directory - this prevents race conditions */
static int mount_ok(char *mount_point)
{
	struct stat st;

	if (chdir(mount_point) != 0) {
		return -1;
	}

        if (stat(".", &st) != 0) {
		return -1;
        }

        if (!S_ISDIR(st.st_mode)) {
                errno = ENOTDIR;
                return -1;
        }

        if ((getuid() != 0) && 
	    ((getuid() != st.st_uid) || 
	     ((st.st_mode & S_IRWXU) != S_IRWXU))) {
                errno = EPERM;
                return -1;
        }

        return 0;
}

/* Tries to mount using the appropriate format. For 2.2 the struct,
   for 2.4 the ascii version. */
static int
do_mount(char *share_name, unsigned int flags, struct smb_mount_data *data)
{
	pstring opts;
	struct utsname uts;
	char *release, *major, *minor;
	char *data1, *data2;

	uname(&uts);
	release = uts.release;
	major = strtok(release, ".");
	minor = strtok(NULL, ".");
	if (major && minor && atoi(major) == 2 && atoi(minor) < 4) {
		/* < 2.4, assume struct */
		data1 = (char *) data;
		data2 = opts;
	} else {
		/* >= 2.4, assume ascii but fall back on struct */
		data1 = opts;
		data2 = (char *) data;
	}

	slprintf(opts, sizeof(opts)-1,
		 "version=7,uid=%d,gid=%d,file_mode=0%o,dir_mode=0%o,%s",
		 mount_uid, mount_gid, data->file_mode, data->dir_mode,options);
	if (mount(share_name, ".", "smbfs", flags, data1) == 0)
		return 0;
	return mount(share_name, ".", "smbfs", flags, data2);
}

 int main(int argc, char *argv[])
{
	char *mount_point, *share_name = NULL;
	FILE *mtab;
	int fd;
	unsigned int flags;
	struct smb_mount_data data;
	struct mntent ment;

	memset(&data, 0, sizeof(struct smb_mount_data));

	if (argc < 2) {
		help();
		exit(1);
	}

	if (argv[1][0] == '-') {
		help();
		exit(1);
	}

	if (getuid() != 0) {
		user_mount = 1;
	}

        if (geteuid() != 0) {
                fprintf(stderr, "smbmnt must be installed suid root for direct user mounts (%d,%d)\n", getuid(), geteuid());
                exit(1);
        }

	mount_uid = getuid();
	mount_gid = getgid();
	mount_fmask = umask(0);
        umask(mount_fmask);
	mount_fmask = ~mount_fmask;

        mount_point = fullpath(argv[1]);

        argv += 1;
        argc -= 1;

        if (mount_ok(mount_point) != 0) {
                fprintf(stderr, "cannot mount on %s: %s\n",
                        mount_point, strerror(errno));
                exit(1);
        }

	data.version = SMB_MOUNT_VERSION;

        /* getuid() gives us the real uid, who may umount the fs */
        data.mounted_uid = getuid();

        if (parse_args(argc, argv, &data, &share_name) != 0) {
                help();
                return -1;
        }

        data.uid = mount_uid;    // truncates to 16-bits here!!!
        data.gid = mount_gid;
        data.file_mode = (S_IRWXU|S_IRWXG|S_IRWXO) & mount_fmask;
        data.dir_mode  = (S_IRWXU|S_IRWXG|S_IRWXO) & mount_dmask;

        if (mount_dmask == 0) {
                data.dir_mode = data.file_mode;
                if ((data.dir_mode & S_IRUSR) != 0)
                        data.dir_mode |= S_IXUSR;
                if ((data.dir_mode & S_IRGRP) != 0)
                        data.dir_mode |= S_IXGRP;
                if ((data.dir_mode & S_IROTH) != 0)
                        data.dir_mode |= S_IXOTH;
        }

	flags = MS_MGC_VAL | MS_NOSUID | MS_NODEV;

	if (mount_ro) flags |= MS_RDONLY;

	if (do_mount(share_name, flags, &data) < 0) {
		switch (errno) {
		case ENODEV:
			fprintf(stderr, "ERROR: smbfs filesystem not supported by the kernel\n");
			break;
		default:
			perror("mount error");
		}
		fprintf(stderr, "Please refer to the smbmnt(8) manual page\n");
		return -1;
	}

        ment.mnt_fsname = share_name ? share_name : "none";
        ment.mnt_dir = mount_point;
        ment.mnt_type = "smbfs";
        ment.mnt_opts = "";
        ment.mnt_freq = 0;
        ment.mnt_passno= 0;

        mount_point = ment.mnt_dir;

	if (mount_point == NULL)
	{
		fprintf(stderr, "Mount point too long\n");
		return -1;
	}
	
        if ((fd = open(MOUNTED"~", O_RDWR|O_CREAT|O_EXCL, 0600)) == -1)
        {
                fprintf(stderr, "Can't get "MOUNTED"~ lock file");
                return 1;
        }
        close(fd);
	
        if ((mtab = setmntent(MOUNTED, "a+")) == NULL)
        {
                fprintf(stderr, "Can't open " MOUNTED);
                return 1;
        }

        if (addmntent(mtab, &ment) == 1)
        {
                fprintf(stderr, "Can't write mount entry");
                return 1;
        }
        if (fchmod(fileno(mtab), 0644) == -1)
        {
                fprintf(stderr, "Can't set perms on "MOUNTED);
                return 1;
        }
        endmntent(mtab);

        if (unlink(MOUNTED"~") == -1)
        {
                fprintf(stderr, "Can't remove "MOUNTED"~");
                return 1;
        }

	return 0;
}	

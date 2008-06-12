#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

static void usage(void)
{
	printf("
smbw_sample - a sample program that uses smbw

smbw_sample <options> path

  options:
     -W workgroup
     -l logfile
     -P prefix
     -d debuglevel
     -U username%%password
     -R resolve order

note that path must start with /smb/
");
}

int main(int argc, char *argv[])
{
	DIR *dir;
	struct dirent *dent;
	int opt;
	char *p;
	extern char *optarg;
	extern int optind;
	char *path;

	lp_load(dyn_CONFIGFILE,1,0,0,1);
	smbw_setup_shared();

	while ((opt = getopt(argc, argv, "W:U:R:d:P:l:hL:")) != EOF) {
		switch (opt) {
		case 'W':
			smbw_setshared("WORKGROUP", optarg);
			break;
		case 'l':
			smbw_setshared("LOGFILE", optarg);
			break;
		case 'P':
			smbw_setshared("PREFIX", optarg);
			break;
		case 'd':
			smbw_setshared("DEBUG", optarg);
			break;
		case 'U':
			p = strchr_m(optarg,'%');
			if (p) {
				*p=0;
				smbw_setshared("PASSWORD",p+1);
			}
			smbw_setshared("USER", optarg);
			break;
		case 'R':
			smbw_setshared("RESOLVE_ORDER",optarg);
			break;
		case 'h':
		default:
			usage();
			exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		exit(1);
	}

	path = argv[0];

	smbw_init();

	dir = smbw_opendir(path);
	if (!dir) {
		printf("failed to open %s\n", path);
		exit(1);
	}
	
	while ((dent = smbw_readdir(dir))) {
		printf("%s\n", dent->d_name);
	}
	smbw_closedir(dir);
	return 0;
}

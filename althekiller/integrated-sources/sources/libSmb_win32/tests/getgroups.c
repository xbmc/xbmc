/* this tests whether getgroups actually returns lists of integers
   rather than gid_t. The test only works if the user running
   the test is in at least 1 group 

   The test is designed to check for those broken OSes that define
   getgroups() as returning an array of gid_t but actually return a
   array of ints! Ultrix is one culprit
  */

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <grp.h>

main()
{
	int i;
	int *igroups;
	char *cgroups;
	int grp = 0;
	int  ngroups = getgroups(0,&grp);

	if (sizeof(gid_t) == sizeof(int)) {
		fprintf(stderr,"gid_t and int are the same size\n");
		exit(1);
	}

	if (ngroups <= 0)
		ngroups = 32;

	igroups = (int *)malloc(sizeof(int)*ngroups);

	for (i=0;i<ngroups;i++)
		igroups[i] = 0x42424242;

	ngroups = getgroups(ngroups,(gid_t *)igroups);

	if (igroups[0] == 0x42424242)
		ngroups = 0;

	if (ngroups == 0) {
		printf("WARNING: can't determine getgroups return type\n");
		exit(1);
	}
	
	cgroups = (char *)igroups;

	if (ngroups == 1 && 
	    cgroups[2] == 0x42 && cgroups[3] == 0x42) {
		fprintf(stderr,"getgroups returns gid_t\n");
		exit(1);
	}
	  
	for (i=0;i<ngroups;i++) {
		if (igroups[i] == 0x42424242) {
			fprintf(stderr,"getgroups returns gid_t\n");
			exit(1);
		}
	}

	exit(0);
}

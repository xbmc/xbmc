/* this test should find out what quota api is available on the os */

 int autoconf_quota(void);

#if defined(HAVE_QUOTACTL_4A)
/* long quotactl(int cmd, char *special, qid_t id, caddr_t addr) */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_ASM_TYPES_H
#include <asm/types.h>
#endif

#if defined(HAVE_LINUX_QUOTA_H)
# include <linux/quota.h>
# if defined(HAVE_STRUCT_IF_DQBLK)
#  define SYS_DQBLK if_dqblk
# elif defined(HAVE_STRUCT_MEM_DQBLK)
#  define SYS_DQBLK mem_dqblk
# endif
#elif defined(HAVE_SYS_QUOTA_H)
# include <sys/quota.h>
#endif

#ifndef SYS_DQBLK
#define SYS_DQBLK dqblk
#endif

 int autoconf_quota(void);

 int autoconf_quota(void)
{
	int ret = -1;
	struct SYS_DQBLK D;

	ret = quotactl(Q_GETQUOTA,"/dev/hda1",0,(void *)&D);
	
	return ret;
}

#elif defined(HAVE_QUOTACTL_4B)
/* int quotactl(const char *path, int cmd, int id, char *addr); */

#ifdef HAVE_SYS_QUOTA_H
#include <sys/quota.h>
#else /* *BSD */
#include <sys/types.h>
#include <ufs/ufs/quota.h>
#include <machine/param.h>
#endif

 int autoconf_quota(void)
{
	int ret = -1;
	struct dqblk D;

	ret = quotactl("/",Q_GETQUOTA,0,(char *) &D);

	return ret;
}

#elif defined(HAVE_QUOTACTL_3)
/* int quotactl (char *spec, int request, char *arg); */

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_QUOTA_H
#include <sys/quota.h>
#endif

 int autoconf_quota(void)
{
	int ret = -1;
	struct q_request request;

	ret = quotactl("/", Q_GETQUOTA, &request);

	return ret;
}

#elif defined(HAVE_QUOTACTL_2)

#error HAVE_QUOTACTL_2 not implemented

#else

#error Unknow QUOTACTL prototype

#endif

 int main(void)
{	
	autoconf_quota();
	return 0;
}

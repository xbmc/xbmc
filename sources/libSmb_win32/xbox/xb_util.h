#ifndef XBOX_SMB_UTIL_H
#define XBOX_SMB_UTIL_H

int ftruncate(int fd, long size);
#define getcwd(buf, size) smb_getcwd(buf, size)
#define chdir(path) smb_chdir(path)
#define getpid() smb_getpid()
#define getgid() smb_getgid()
#define getegid() smb_getgid()
#define getuid() smb_getuid()
#define geteuid() smb_getuid()
#define setgid(int) smb_setgid(int)
#define setuid(int) smb_setuid(int)


int gettimeofday(struct timeval *tp, void *tzp);

char *smb_getcwd(char *buf, int size);
int smb_chdir(const char *path);

int strcasecmp(const char *s1, const char *s2);

int smb_getgid();
int smb_getuid();
void smb_setgid(int id);
void smb_setuid(int id);

int smb_getpid();
int waitpid(int,int*, int);
int fork();

struct group *getgrent(void);
void setgrent(void);
void endgrent(void);

int getgroups(int size, gid_t list[]);
int setgroups(size_t size, const gid_t *list);

//#define __setpwent() setpwent()
//#define __getpwent() getpwent()
//#define __endpwent() endpwent()

struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);

#define getpwuid(_uid) __getpwuid(_uid)
#define getpwnam(_name) __getpwnam(_name)

struct passwd *getpwnam(const char * name);
struct passwd *getpwuid(uid_t uid);

struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);

int random();

#define HAVE_PWD_H 1
#define	_PATH_PASSWD		"/etc/passwd"
#define	_PASSWORD_LEN		128	/* max length, not counting NULL */

void SMB_Output(const char* pszFormat, ...);


struct passwd {
	char	*pw_name;		/* user name */
	char	*pw_passwd;		/* encrypted password */
	int	pw_uid;			/* user uid */
	int	pw_gid;			/* user gid */
	char	*pw_comment;		/* comment */
	char	*pw_gecos;		/* Honeywell login info */
	char	*pw_dir;		/* home directory */
	char	*pw_shell;		/* default shell */
};

#endif // XBOX_SMB_UTIL_H

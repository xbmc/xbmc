#ifndef __DLFCN_H__
# define __DLFCN_H__
/*
 * $Id: dlfcn.h,v 1.1 2003/04/29 20:21:35 tchamp Exp $
 * $Name:  $
 * 
 *
 */
extern void *dlopen  (const char *file, int mode);
extern int   dlclose (void *handle);
extern void *dlsym   (void * handle, const char * name);
extern char *dlerror (void);

/* These don't mean anything on windows */
#define RTLD_NEXT      ((void *) -1l)
#define RTLD_DEFAULT   ((void *) 0)
#define RTLD_LAZY					-1
#define RTLD_NOW					-1
#define RTLD_BINDING_MASK -1
#define RTLD_NOLOAD				-1
#define RTLD_GLOBAL				-1

#endif /* __DLFCN_H__ */

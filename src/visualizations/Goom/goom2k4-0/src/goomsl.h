#ifndef _GOOMSL_H
#define _GOOMSL_H

#include "goomsl_hash.h"

typedef struct _GoomSL GoomSL;
typedef void (*GoomSL_ExternalFunction)(GoomSL *gsl, GoomHash *global_vars, GoomHash *local_vars);

GoomSL*gsl_new(void);
void   gsl_free(GoomSL *gss);

char *gsl_init_buffer(const char *file_name);
void  gsl_append_file_to_buffer(const char *file_name, char **buffer);

void   gsl_compile (GoomSL *scanner, const char *script);
void   gsl_execute (GoomSL *scanner);
int    gsl_is_compiled  (GoomSL *gss);
void   gsl_bind_function(GoomSL *gss, const char *fname, GoomSL_ExternalFunction func);

int    gsl_malloc  (GoomSL *_this, int size);
void  *gsl_get_ptr (GoomSL *_this, int id);
void   gsl_free_ptr(GoomSL *_this, int id);

GoomHash *gsl_globals(GoomSL *_this);

#define GSL_LOCAL_PTR(gsl,local,name)   gsl_get_ptr(gsl, *(int*)goom_hash_get(local,name)->ptr)
#define GSL_LOCAL_INT(gsl,local,name)   (*(int*)goom_hash_get(local,name)->ptr)
#define GSL_LOCAL_FLOAT(gsl,local,name) (*(float*)goom_hash_get(local,name)->ptr)

#define GSL_GLOBAL_PTR(gsl,name)   gsl_get_ptr(gsl, *(int*)goom_hash_get(gsl_globals(gsl),name)->ptr)
#define GSL_GLOBAL_INT(gsl,name)   (*(int*)goom_hash_get(gsl_globals(gsl),name)->ptr)
#define GSL_GLOBAL_FLOAT(gsl,name) (*(float*)goom_hash_get(gsl_globals(gsl),name)->ptr)

#endif

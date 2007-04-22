
#ifndef JS_H
#define JS_H

#include "../links.h"

#define TRACE fprintf(stderr,"CALL: %s, %s:%d\n",__FILE__,__FUNCTION__,__LINE__)

/* javascript.c */

#define MAXTIMERS 100

#define STACK_CHUNK_SIZE 8192

struct javascript_context *js_create_context(void *, long);
void js_destroy_context(struct javascript_context *);
void js_execute_code(struct javascript_context *, unsigned char *, int, void (*)(void *));

struct session;
struct f_data;
struct f_data_c;

void javascript_func(struct session *ses, unsigned char *code);
void jsint_execute_code(struct f_data_c *, unsigned char *, int, int, int, int);
void jsint_destroy(struct f_data_c *);
void jsint_run_queue(struct f_data_c *);
int jsint_get_source(struct f_data_c *, unsigned char **, unsigned char **);
void jsint_scan_script_tags(struct f_data_c *);
void jsint_destroy_document_description(struct f_data *);

#endif

/*
 * sys/queue.h wrappers and helpers
 */

#ifndef HTSQ_H
#define HTSQ_H

#ifdef _MSC_VER
#undef SLIST_ENTRY
#include "Win32/include/sys/queue.h"
#else
#include <sys/queue.h>
#endif

/*
 * Complete missing LIST-ops
 */

#ifndef LIST_FOREACH
#define	LIST_FOREACH(var, head, field)					\
	for ((var) = ((head)->lh_first);				\
		(var);							\
		(var) = ((var)->field.le_next))
#endif

#ifndef LIST_EMPTY
#define	LIST_EMPTY(head)		((head)->lh_first == NULL)
#endif

#ifndef LIST_FIRST
#define	LIST_FIRST(head)		((head)->lh_first)
#endif

#ifndef LIST_NEXT
#define	LIST_NEXT(elm, field)		((elm)->field.le_next)
#endif

#ifndef LIST_INSERT_BEFORE
#define	LIST_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.le_prev = (listelm)->field.le_prev;		\
	(elm)->field.le_next = (listelm);				\
	*(listelm)->field.le_prev = (elm);				\
	(listelm)->field.le_prev = &(elm)->field.le_next;		\
} while (/*CONSTCOND*/0)
#endif

/*
 * Complete missing TAILQ-ops
 */

#ifndef TAILQ_INSERT_BEFORE
#define	TAILQ_INSERT_BEFORE(listelm, elm, field) do {			\
	(elm)->field.tqe_prev = (listelm)->field.tqe_prev;		\
	(elm)->field.tqe_next = (listelm);				\
	*(listelm)->field.tqe_prev = (elm);				\
	(listelm)->field.tqe_prev = &(elm)->field.tqe_next;		\
} while (0)
#endif

#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)                                     \
 for ((var) = ((head)->tqh_first); (var); (var) = ((var)->field.tqe_next))
#endif

#ifndef TAILQ_FIRST
#define TAILQ_FIRST(head)               ((head)->tqh_first)
#endif

#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elm, field)          ((elm)->field.tqe_next)
#endif

#ifndef TAILQ_LAST
#define TAILQ_LAST(head, headname) \
        (*(((struct headname *)((head)->tqh_last))->tqh_last))
#endif

#ifndef TAILQ_PREV
#define TAILQ_PREV(elm, headname, field) \
        (*(((struct headname *)((elm)->field.tqe_prev))->tqh_last))
#endif

#ifndef TAILQ_FOREACH_REVERSE
#define	TAILQ_FOREACH_REVERSE(var, head, headname, field)		\
	for ((var) = (*(((struct headname *)((head)->tqh_last))->tqh_last));	\
		(var);							\
		(var) = (*(((struct headname *)((var)->field.tqe_prev))->tqh_last)))
#endif

/*
 * Some extra functions for LIST manipulation
 */

#define LIST_MOVE(newhead, oldhead, field) do {			        \
        if((oldhead)->lh_first) {					\
           (oldhead)->lh_first->field.le_prev = &(newhead)->lh_first;	\
	}								\
        (newhead)->lh_first = (oldhead)->lh_first;			\
} while (0) 

#define LIST_INSERT_SORTED(head, elm, field, cmpfunc) do {	\
        if(LIST_EMPTY(head)) {					\
           LIST_INSERT_HEAD(head, elm, field);			\
        } else {						\
           typeof(elm) _tmp;					\
           LIST_FOREACH(_tmp,head,field) {			\
              if(cmpfunc(elm,_tmp) <= 0) {			\
                LIST_INSERT_BEFORE(_tmp,elm,field);		\
                break;						\
              }							\
              if(!LIST_NEXT(_tmp,field)) {			\
                 LIST_INSERT_AFTER(_tmp,elm,field);		\
                 break;						\
              }							\
           }							\
        }							\
} while(0)

#define TAILQ_INSERT_SORTED(head, elm, field, cmpfunc) do {	\
        if(TAILQ_FIRST(head) == NULL) {				\
           TAILQ_INSERT_HEAD(head, elm, field);			\
        } else {						\
           typeof(elm) _tmp;					\
           TAILQ_FOREACH(_tmp,head,field) {			\
              if(cmpfunc(elm,_tmp) <= 0) {			\
                TAILQ_INSERT_BEFORE(_tmp,elm,field);		\
                break;						\
              }							\
              if(!TAILQ_NEXT(_tmp,field)) {			\
                 TAILQ_INSERT_AFTER(head,_tmp,elm,field);	\
                 break;						\
              }							\
           }							\
        }							\
} while(0)

#define TAILQ_MOVE(newhead, oldhead, field) do { \
        if(TAILQ_FIRST(oldhead)) { \
           TAILQ_FIRST(oldhead)->field.tqe_prev = &(newhead)->tqh_first;  \
        } \
        (newhead)->tqh_first = (oldhead)->tqh_first;                   \
        (newhead)->tqh_last = (oldhead)->tqh_last;                     \
} while (/*CONSTCOND*/0) 
 

#endif /* HTSQ_H */

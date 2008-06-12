#ifndef __REFMEM_LOCAL_H
#define __REFMEM_LOCAL_H

/*
 * Debug level constants used to determine the level of debug tracing
 * to be done and the debug level of any given message.
 */

#define REF_DBG_NONE  -1
#define REF_DBG_DEBUG  0
#define REF_DBG_ALL    0

void refmem_dbg_level(int l);

void refmem_dbg_all();

void refmem_dbg_none();

void refmem_dbg(int level, char *fmt, ...);
#endif /* __REFMEM_LOCAL_H */

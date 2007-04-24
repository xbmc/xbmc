#ifndef _RAR_RESOURCE_
#define _RAR_RESOURCE_

#if defined(SILENT) && defined(RARDLL)
#define St(x) ("")
#else
const char *St(MSGID StringId);
#endif


inline const char *StT(MSGID StringId) {return(St(StringId));}


#endif

#include "rar.hpp"



#if !defined(SILENT) || !defined(RARDLL)
const char *St(MSGID StringId)
{
  return(StringId);
}
#endif

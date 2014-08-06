#ifndef _GUIPLEXDEFAULTACTIONHANDLER_
#define _GUIPLEXDEFAULTACTIONHANDLER_

#include "Key.h"
#include "FileItem.h"

class CGUIPlexDefaultActionHandler
{
public:
  CGUIPlexDefaultActionHandler() {}
  
  static bool OnAction(CAction action, CFileItemPtr item);
};

#endif /* _GUIPLEXDEFAULTACTIONHANDLER_ */

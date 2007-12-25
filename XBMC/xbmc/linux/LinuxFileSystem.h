#ifndef LINUX_FILESYSTEM_H
#define LINUX_FILESYSTEM_H

#include <vector>
#include "StdString.h"

class CLinuxFileSystem
{
public:
   static vector<CStdString> GetRemoveableDrives();
};

#endif

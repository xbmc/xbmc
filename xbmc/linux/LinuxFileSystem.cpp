#include "LinuxFileSystem.h"
#include "RegExp.h"

// Example of what pmount returns
// /dev/sdc1 on /media/YUVAL'S DOK type vfat (rw,nosuid,nodev,shortname=mixed,uid=1000,utf8,umask=077,usefree)

vector<CStdString> CLinuxFileSystem::GetRemoveableDrives()
{
   vector<CStdString> result;
   
   CRegExp reMount;
   reMount.RegComp("on (.+) type ([^ ]+)");
   
   char line[1024];
   
   FILE* pipe = popen("pmount", "r");
   if (pipe)
   {
      while (fgets(line, sizeof(line) - 1, pipe))
      {
         if (reMount.RegFind(line) != -1)
         {
            char* mount = reMount.GetReplaceString("\\1");
            char* fs = reMount.GetReplaceString("\\2");

            // ignore iso9660 as we support it natively in XBMC, although 
            // this is a better way 
            if (strcmp(fs, "iso9660") == 0)
               continue;

            result.push_back(mount);

            free(fs);
            free(mount);
         }
      }
       
      pclose(pipe);
   }   
   
   return result;
}

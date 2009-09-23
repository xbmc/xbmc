#include <stdio.h>
#include "mms.h"

const char *url = "mms://od-msn.msn.com/3/mbr/apprentice_bts.wmv";

int main(int argc, char *argv[])
{
  mms_t *this = NULL;
  char buf[1024];
  int i, res;
  FILE* f;
  
  if((this = mms_connect(NULL, NULL, url, 1)))
    printf("Connect OK\n");
  f = fopen("/tmp/mmsdownload.test", "w");
  for(i = 0; i < 10000; i++)
  {
    res = mms_read(NULL, this, buf, 1024);
    if(!res)
      break;
    fwrite(buf, 1, res, f);
  }
  if(i > 0)
    printf("OK, read %d times\n", i);
  else
    printf("Failed to read from stream\n");
  
  
}

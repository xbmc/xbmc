#include "mms.h"

const char *url = "mms://od-msn.msn.com/3/mbr/apprentice_bts.wmv";

int main(int argc, char *argv[])
{
  if(mms_connect(NULL, NULL, url, 1))
    printf("Connect OK\n");

  return 0;
}

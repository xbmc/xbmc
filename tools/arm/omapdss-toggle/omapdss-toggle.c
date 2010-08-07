#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#define ALPHA_BLENDNING_TOGGLE "/sys/devices/platform/omapdss/manager0/alpha_blending_enabled"

void PrintHelp(const char *Prog)
{
  printf("Commands:\n");
  printf("\t--disable-alphablend\n");
  printf("\t--enable-alphablend\n");
}

int main(int argc, char **argv)
{
  int enableAlphaBlend = -1;

  int i = 0;
  for (i = 0; i < argc; i++)
  {
    if (strcmp(argv[i], "--help") == 0)
    {
      PrintHelp(argv[0]);
      return 0;
    }
    else if (strcmp(argv[i], "--disable-alphablend") == 0)
      enableAlphaBlend = 0;
    else if (strcmp(argv[i], "--enable-alphablend") == 0)
      enableAlphaBlend = 1;
  }

  if (enableAlphaBlend >= 0)
  {
    FILE *file = fopen(ALPHA_BLENDNING_TOGGLE, "w");
    if (file == NULL)
    {
      printf("Failed to set alpha blendning\n");
      return 1;
    }
    else
    {
      const char *data = enableAlphaBlend == 0 ? "0" : "1";
      fwrite(data, sizeof(char), 1, file);
      fclose(file);
      return 0;
    }
  }

  return 0;
}

#include <stdio.h>
#define NO_CHR 0xffff

int
main(int argc, char *argv[])
{
  unsigned int target_map[0x100];
  unsigned int source_map[0x100];
  unsigned char xlt[0x100];
  unsigned int ucs2_map[0x10000];
  FILE *f;

  int i;
  int c;

  f = fopen(argv[1], "r");
  for (i = 0; i < 0x100; i++)
    fscanf(f, "%x", source_map + i);
  fclose(f);

  f = fopen(argv[2], "r");
  for (i = 0; i < 0x100; i++)
    fscanf(f, "%x", target_map + i);
  fclose(f);

  for (i = 0; i < 0x100; i++)
    xlt[i] = (unsigned char)i;

  for (i = 0; i < 0x10000; i++)
    ucs2_map[i] = NO_CHR;

  for (i = 0xff; i >= 0; i--) {
    if (target_map[i] != NO_CHR)
      ucs2_map[target_map[i]] = (unsigned int)i;
  }

  for (i = 0xff; i >= 0; i--) {
    if (source_map[i] != NO_CHR
        && ucs2_map[source_map[i]] != NO_CHR)
      xlt[i] = (unsigned char)ucs2_map[source_map[i]];
  }

  while ((c = getchar()) != EOF)
    putchar(xlt[c]);

  return 0;
}
/* vim: ts=2
 */

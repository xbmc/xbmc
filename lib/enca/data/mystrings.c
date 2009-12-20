#include <ctype.h>
#include <stdio.h>

#define isbinary(x) (iscntrl(x) && !isspace(x))
#define istext(x) (islower(x) || isupper(x) || (x) > 127)

#define MINTEXT 4

#define FILL_CHARACTER ' '

int
main(void)
{
  int mode;
  unsigned char old[MINTEXT - 1];
  int c;

  mode = 0;
  while ((c = getchar()) != EOF) {
    if (isbinary(c)) {
      if (mode == 0) putchar('\n');
      mode = MINTEXT;
    }
    else {
      if (mode > 0) {
        if (istext(c)) {
          mode--;
          if (mode == 0) {
            int j;
            for (j = 0; j < MINTEXT-1; j++)
              putchar(old[j]);

            putchar(c);
          }
          else old[MINTEXT - mode - 1] = c;
        }
        else mode = MINTEXT;
      }
      else putchar(c);
    }
  }
  if (mode == 0) putchar('\n');

  return 0;
}

/* Prints the portable name for the current locale's charset. */

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include "localcharset.h"

int main ()
{
  setlocale(LC_ALL, "");
  printf("%s\n", locale_charset());
  exit(0);
}

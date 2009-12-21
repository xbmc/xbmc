/* Prints the locale's encoding via libX11. */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

int main (int argc, char* argv[])
{
  Display* display;
  XTextProperty textprop;
  char* input;

  if (argc != 1)
    exit(1);

  setlocale(LC_CTYPE,"");

  display = XOpenDisplay(NULL);
  if (display == NULL) {
    fprintf(stderr,"cannot open display\n");
    exit(1);
  }

  input = "";
  if (XmbTextListToTextProperty(display, &input, 1, XTextStyle, &textprop) != Success) {
    fprintf(stderr,"XmbTextListToTextProperty failed\n");
    exit(1);
  }
  assert(textprop.format == 8);
  assert(textprop.nitems == 0);

  printf("%s\n", XGetAtomName(display, textprop.encoding));

  XCloseDisplay(display);

  exit(0);
}

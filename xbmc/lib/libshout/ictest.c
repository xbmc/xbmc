#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <iconv.h>
#include <wchar.h>
#include <wctype.h>

int
main (int argc, char* argv[])
{
    wchar_t buf[1024];

    setlocale (LC_CTYPE, "");
    swprintf (buf, 1024, L"%ls", L"HiHi");
    printf ("%d wide chars in arg\n", wcslen(L"HiHi"));
    printf ("%d wide chars in buf\n", wcslen(buf));
    return 0;
}


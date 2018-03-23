/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2016 Team Kodi
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *-----------------------------------------------------------------------*/

#include <stdlib.h>

#include "win32_utils.h"

wchar_t* to_utf16(const char* str, size_t length)
{
  if (length == 0)
    length = strlen(str);
  int result = MultiByteToWideChar(CP_UTF8, 0, str, length, NULL, 0);
  if (result == 0)
  {
    return NULL;
  }

  int newLen = result + 1;
  wchar_t* dirPath = malloc(newLen * 2);
  result = MultiByteToWideChar(CP_UTF8, 0, str, length, dirPath, newLen);

  if (result == 0)
  {
    free(dirPath);
    return NULL;
  }

  dirPath[newLen - 1] = L'\0';
  return dirPath;
}

char* to_utf8(const wchar_t* str, size_t length)
{
  if (length == 0)
    length = wcslen(str);

  int result = WideCharToMultiByte(CP_UTF8, 0, str, length, NULL, 0, NULL, NULL);
  if (result == 0)
    return NULL;

  int newLen = result + 1;
  char *newStr = malloc(newLen);
  result = WideCharToMultiByte(CP_UTF8, 0, str, length, newStr, result, NULL, NULL);
  if (result == 0)
  {
    free(newStr);
    return NULL;
  }

  newStr[newLen - 1] = '\0';

  return newStr;
}

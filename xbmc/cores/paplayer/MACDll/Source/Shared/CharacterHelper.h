/*******************************************************************************************
Character set conversion helpers
*******************************************************************************************/

#ifndef CHARACTER_HELPER_H
#define CHARACTER_HELPER_H

str_ansi * GetANSIFromUTF8(const str_utf8 * pUTF8);
str_ansi * GetANSIFromUTF16(const str_utf16 * pUTF16);
str_utf16 * GetUTF16FromANSI(const str_ansi * pANSI);
str_utf16 * GetUTF16FromUTF8(const str_utf8 * pUTF8);
str_utf8 * GetUTF8FromANSI(const str_ansi * pANSI);
str_utf8 * GetUTF8FromUTF16(const str_utf16 * pUTF16);

#endif // #ifndef CHARACTER_HELPER_H


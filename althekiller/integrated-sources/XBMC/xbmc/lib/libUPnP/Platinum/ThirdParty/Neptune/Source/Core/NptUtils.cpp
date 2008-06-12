/*****************************************************************
|
|   Neptune - Utils
|
|   (c) 2001-2003 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <math.h>

#include "NptConfig.h"
#include "NptDebug.h"
#include "NptUtils.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_FORMAT_LOCAL_BUFFER_SIZE = 1024;
const unsigned int NPT_FORMAT_BUFFER_INCREMENT  = 4096;
const unsigned int NPT_FORMAT_BUFFER_MAX_SIZE   = 65536;

/*----------------------------------------------------------------------
|   NPT_BytesToInt32Be
+---------------------------------------------------------------------*/
NPT_UInt32 
NPT_BytesToInt32Be(const unsigned char* bytes)
{
    return 
        ( ((NPT_UInt32)bytes[0])<<24 ) |
        ( ((NPT_UInt32)bytes[1])<<16 ) |
        ( ((NPT_UInt32)bytes[2])<<8  ) |
        ( ((NPT_UInt32)bytes[3])     );    
}

/*----------------------------------------------------------------------
|   NPT_BytesToInt16Be
+---------------------------------------------------------------------*/
NPT_UInt16
NPT_BytesToInt16Be(const unsigned char* bytes)
{
    return 
        ( ((NPT_UInt16)bytes[0])<<8  ) |
        ( ((NPT_UInt16)bytes[1])     );    
}

/*----------------------------------------------------------------------
|    NPT_BytesFromInt32Be
+---------------------------------------------------------------------*/
void 
NPT_BytesFromInt32Be(unsigned char* buffer, NPT_UInt32 value)
{
    buffer[0] = (unsigned char)(value>>24) & 0xFF;
    buffer[1] = (unsigned char)(value>>16) & 0xFF;
    buffer[2] = (unsigned char)(value>> 8) & 0xFF;
    buffer[3] = (unsigned char)(value    ) & 0xFF;
}

/*----------------------------------------------------------------------
|    NPT_BytesFromInt16Be
+---------------------------------------------------------------------*/
void 
NPT_BytesFromInt16Be(unsigned char* buffer, NPT_UInt16 value)
{
    buffer[0] = (unsigned char)((value>> 8) & 0xFF);
    buffer[1] = (unsigned char)((value    ) & 0xFF);
}

#if !defined(NPT_CONFIG_HAVE_SNPRINTF)
/*----------------------------------------------------------------------
|   NPT_FormatString
+---------------------------------------------------------------------*/
int 
NPT_FormatString(char* /*str*/, NPT_Size /*size*/, const char* /*format*/, ...)
{
    NPT_ASSERT(0); // not implemented yet
    return 0;
}
#endif // NPT_CONFIG_HAVE_SNPRINTF

/*----------------------------------------------------------------------
|   NPT_NibbleToHex
+---------------------------------------------------------------------*/
static char NPT_NibbleToHex(unsigned int nibble)
{
    NPT_ASSERT(nibble < 16);
    return (nibble < 10) ? ('0' + nibble) : ('A' + (nibble-10));
}

/*----------------------------------------------------------------------
|   NPT_HexToNibble
+---------------------------------------------------------------------*/
static int NPT_HexToNibble(char hex)
{
    if (hex >= 'a' && hex <= 'f') {
        return ((hex - 'a') + 10);
    } else if (hex >= 'A' && hex <= 'F') {
        return ((hex - 'A') + 10);
    } else if (hex >= '0' && hex <= '9') {
        return (hex - '0');
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------
|   NPT_ByteToHex
+---------------------------------------------------------------------*/
void
NPT_ByteToHex(NPT_Byte b, char* buffer)
{
    buffer[0] = NPT_NibbleToHex((b>>4) & 0x0F);
    buffer[1] = NPT_NibbleToHex(b      & 0x0F);
}

/*----------------------------------------------------------------------
|   NPT_HexToByte
+---------------------------------------------------------------------*/
NPT_Result
NPT_HexToByte(const char* buffer, NPT_Byte& b)
{
    int nibble_0 = NPT_HexToNibble(buffer[0]);
    if (nibble_0 < 0) return NPT_ERROR_INVALID_SYNTAX;
    
    int nibble_1 = NPT_HexToNibble(buffer[1]);
    if (nibble_1 < 0) return NPT_ERROR_INVALID_SYNTAX;

    b = (nibble_0 << 4) | nibble_1;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NPT_ParseInteger
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ParseInteger(const char* str, long& result, bool relaxed, NPT_Cardinal* chars_used)
{
    // safe default value
    result = 0;
    if (chars_used) *chars_used = 0;

    if (str == NULL) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // ignore leading whitespace
    if (relaxed) {
        while (*str == ' ' || *str == '\t') {
            str++;
            if (chars_used) (*chars_used)++;
        }
    }
    if (*str == '\0') {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // check for sign
    bool negative = false;
    if (*str == '-') {
        // negative number
        negative = true; 
        str++;
        if (chars_used) (*chars_used)++;
    } else if (*str == '+') {
        // skip the + sign
        str++;
        if (chars_used) (*chars_used)++;
    }

    // parse the digits
    bool empty      = true;
    NPT_Int32 value = 0;
    char c;
    while ((c = *str++)) {
        if (c >= '0' && c <= '9') {
            value = 10*value + (c-'0');
            empty = false;
            if (chars_used) (*chars_used)++;
        } else {
            if (relaxed) {
                break;
            } else {
                return NPT_ERROR_INVALID_PARAMETERS;
            }
        } 
    }

    // check that the value was non empty
    if (empty) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // return the result
    result = negative ? -value : value;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|    NPT_ParseInteger32
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ParseInteger32(const char* str, NPT_Int32& value, bool relaxed)
{
    long value_l;
    NPT_Result result = NPT_ParseInteger(str, value_l, relaxed);
    if (NPT_SUCCEEDED(result)) value = (NPT_Int32)value_l;
    return result;
}

/*----------------------------------------------------------------------
|    NPT_ParseFloat
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ParseFloat(const char* str, float& result, bool relaxed)
{
    // safe default value 
    result = 0.0f;

    // check params
    if (str == NULL || *str == '\0') {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // ignore leading whitespace
    if (relaxed) {
        while (*str == ' ' || *str == '\t') {
            str++;
        }
    }
    if (*str == '\0') {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // check for sign
    bool  negative = false;
    if (*str == '-') {
        // negative number
        negative = true; 
        str++;
    } else if (*str == '+') {
        // skip the + sign
        str++;
    }

    // parse the digits
    bool  after_radix = false;
    bool  empty = true;
    float value = 0.0f;
    float decimal = 10.0f;
    char  c;
    while ((c = *str++)) {
        if (c == '.') {
            if (after_radix || (*str < '0' || *str > '9')) {
                return NPT_ERROR_INVALID_PARAMETERS;
            } else {
                after_radix = true;
            }
        } else if (c >= '0' && c <= '9') {
            empty = false;
            if (after_radix) {
                value += (float)(c-'0')/decimal;
                decimal *= 10.0f;
            } else {
                value = 10.0f*value + (float)(c-'0');
            }
        } else if (c == 'e' || c == 'E') {
            // exponent
            if (*str == '+' || *str == '-' || (*str >= '0' && *str <= '9')) {
                long exponent = 0;
                if (NPT_SUCCEEDED(NPT_ParseInteger(str, exponent, relaxed))) {
                    value *= (float)pow(10.0f, (float)exponent);
                    break;
                } else {
                    return NPT_ERROR_INVALID_PARAMETERS;
                }
            } else {
                return NPT_ERROR_INVALID_PARAMETERS;
            }
        } else {
            if (relaxed) {
                break;
            } else {
                return NPT_ERROR_INVALID_PARAMETERS;
            }
        } 
    }

    // check that the value was non empty
    if (empty) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // return the result
    result = negative ? -value : value;
    return NPT_SUCCESS;
}

#if !defined(NPT_CONFIG_HAVE_STRCPY)
/*----------------------------------------------------------------------
|    NPT_CopyString
+---------------------------------------------------------------------*/
void
NPT_CopyString(char* dst, const char* src)
{
    while(*dst++ = *src++);
}
#endif

/*----------------------------------------------------------------------
|   NPT_FormatOutput
+---------------------------------------------------------------------*/
void
NPT_FormatOutput(void        (*function)(void* parameter, const char* message),
                 void*       function_parameter,
                 const char* format, 
                 va_list     args)
{
    char         local_buffer[NPT_FORMAT_LOCAL_BUFFER_SIZE];
    unsigned int buffer_size = NPT_FORMAT_LOCAL_BUFFER_SIZE;
    char*        buffer = local_buffer;

    for(;;) {
        int result;

        /* try to format the message (it might not fit) */
        result = NPT_FormatStringVN(buffer, buffer_size-1, format, args);
        buffer[buffer_size-1] = 0; /* force a NULL termination */
        if (result >= 0) break;

        /* the buffer was too small, try something bigger */
        buffer_size = (buffer_size+NPT_FORMAT_BUFFER_INCREMENT)*2;
        if (buffer_size > NPT_FORMAT_BUFFER_MAX_SIZE) break;
        if (buffer != local_buffer) delete[] buffer;
        buffer = new char[buffer_size];
        if (buffer == NULL) return;
    }

    (*function)(function_parameter, buffer);
    if (buffer != local_buffer) delete[] buffer;
}

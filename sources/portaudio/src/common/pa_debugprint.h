#ifndef PA_LOG_H
#define PA_LOG_H
/*
 * Log file redirector function
 * Copyright (c) 1999-2006 Ross Bencina, Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however,
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also
 * requested that these non-binding requests be included along with the
 * license above.
 */

/** @file
 @ingroup common_src
*/


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



void PaUtil_DebugPrint( const char *format, ... );


/*
    The basic format for log messages is described below. If you need to
    add any log messages, please follow this format.

    Function entry (void function):

        "FunctionName called.\n"

    Function entry (non void function):

        "FunctionName called:\n"
        "\tParam1Type param1: param1Value\n"
        "\tParam2Type param2: param2Value\n"      (etc...)


    Function exit (no return value):

        "FunctionName returned.\n"

    Function exit (simple return value):

        "FunctionName returned:\n"
        "\tReturnType: returnValue\n"

    If the return type is an error code, the error text is displayed in ()

    If the return type is not an error code, but has taken a special value
    because an error occurred, then the reason for the error is shown in []

    If the return type is a struct ptr, the struct is dumped.

    See the code below for examples
*/

/** PA_DEBUG() provides a simple debug message printing facility. The macro
 passes it's argument to a printf-like function called PaUtil_DebugPrint()
 which prints to stderr and always flushes the stream after printing.
 Because preprocessor macros cannot directly accept variable length argument
 lists, calls to the macro must include an additional set of parenthesis, eg:
 PA_DEBUG(("errorno: %d", 1001 ));
*/


#ifdef PA_ENABLE_DEBUG_OUTPUT
#define PA_DEBUG(x) PaUtil_DebugPrint x ;
#else
#define PA_DEBUG(x)
#endif


#ifdef PA_LOG_API_CALLS
#define PA_LOGAPI(x) PaUtil_DebugPrint x 

#define PA_LOGAPI_ENTER(functionName) PaUtil_DebugPrint( functionName " called.\n" )

#define PA_LOGAPI_ENTER_PARAMS(functionName) PaUtil_DebugPrint( functionName " called:\n" )

#define PA_LOGAPI_EXIT(functionName) PaUtil_DebugPrint( functionName " returned.\n" )

#define PA_LOGAPI_EXIT_PAERROR( functionName, result ) \
	PaUtil_DebugPrint( functionName " returned:\n" ); \
	PaUtil_DebugPrint("\tPaError: %d ( %s )\n", result, Pa_GetErrorText( result ) )

#define PA_LOGAPI_EXIT_T( functionName, resultFormatString, result ) \
	PaUtil_DebugPrint( functionName " returned:\n" ); \
	PaUtil_DebugPrint("\t" resultFormatString "\n", result )

#define PA_LOGAPI_EXIT_PAERROR_OR_T_RESULT( functionName, positiveResultFormatString, result ) \
	PaUtil_DebugPrint( functionName " returned:\n" ); \
	if( result > 0 ) \
        PaUtil_DebugPrint("\t" positiveResultFormatString "\n", result ); \
    else \
        PaUtil_DebugPrint("\tPaError: %d ( %s )\n", result, Pa_GetErrorText( result ) )
#else
#define PA_LOGAPI(x)
#define PA_LOGAPI_ENTER(functionName)
#define PA_LOGAPI_ENTER_PARAMS(functionName)
#define PA_LOGAPI_EXIT(functionName)
#define PA_LOGAPI_EXIT_PAERROR( functionName, result )
#define PA_LOGAPI_EXIT_T( functionName, resultFormatString, result )
#define PA_LOGAPI_EXIT_PAERROR_OR_T_RESULT( functionName, positiveResultFormatString, result )
#endif

    
typedef void (*PaUtilLogCallback ) (const char *log);

/**
    Install user provided log function
*/
void PaUtil_SetDebugPrintFunction(PaUtilLogCallback  cb);



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* PA_LOG_H */

/*****************************************************************
|
|      Neptune - Environment variables: StdC Implementation
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdlib.h>

#include "NptConfig.h"
#include "NptUtils.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|   NPT_GetEnvironment
+---------------------------------------------------------------------*/
NPT_Result 
NPT_GetEnvironment(const char* name, NPT_String& value)
{
    char* env;

    /* default value */
    value.SetLength(0);

#if defined(NPT_CONFIG_HAVE_GETENV)
    env = getenv(name);
    if (env) {
        value = env;
        return NPT_SUCCESS;
    } else {
        return NPT_ERROR_NO_SUCH_ITEM;
    }
#elif defined(NPT_CONFIG_HAVE_DUPENV_S)
    if (dupenv_s(&env, NULL, name) != 0) {
        return NPT_FAILURE;
    } else if (env != NULL) {
        value = env;
        free(env);
        return NPT_SUCCESS;
    } else {
        return NPT_ERROR_NO_SUCH_ITEM;
    }
#else
#error "no getenv or getenv_s available on this platform"
#endif
}

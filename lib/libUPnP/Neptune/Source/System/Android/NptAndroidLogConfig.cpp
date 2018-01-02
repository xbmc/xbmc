extern "C" {
#include <sys/system_properties.h>
}
#include <android/log.h>
#include <dlfcn.h>
#include "NptLogging.h"

/*----------------------------------------------------------------------
|   android_property_get
+---------------------------------------------------------------------*/
static int android_property_get(const char* name, char* value) {
    static int (*__real_system_property_get)(const char*, char*) = NULL;
    if (__real_system_property_get == NULL) {
        void* handle = dlopen("libc.so", 0);
        if (!handle) {
             __android_log_print(ANDROID_LOG_DEBUG, "Neptune", "Cannot dlopen libc.so: %s", dlerror());
             return 0;
        }
        __real_system_property_get = reinterpret_cast<int (*)(const char*, char*)>(
                                        dlsym(handle, "__system_property_get"));
        if (!__real_system_property_get) {
             __android_log_print(ANDROID_LOG_DEBUG, "Neptune", "Cannot resolve __system_property_get(): %s", dlerror());
             return 0;
        }
    }
    return (*__real_system_property_get)(name, value);
}

/*----------------------------------------------------------------------
|   NPT_GetSystemLogConfig
+---------------------------------------------------------------------*/
NPT_Result
NPT_GetSystemLogConfig(NPT_String& config)
{
    char android_npt_config[PROP_VALUE_MAX];
    android_npt_config[0] = 0;
    int prop_len = android_property_get("persist.neptune_log_config", 
                                         android_npt_config);
    if (prop_len > 0) {
        config = android_npt_config;
        return NPT_SUCCESS;
    } else {
        return NPT_ERROR_NO_SUCH_PROPERTY;
    }
}

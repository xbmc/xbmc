#include <string.h>
#include <sys/types.h>
#include <objc/runtime.h>
#ifdef __cplusplus
#define _default(x) = x
extern "C" {
#else
#define _default(x)
#endif
typedef const void *MSImageRef;
void MSHookFunction(void *symbol, void *replace, void **result);
void *MSFindSymbol(const void *image, const char *name);
MSImageRef MSGetImageByName(const char *file);

#ifdef __APPLE__
#ifdef __arm__
IMP MSHookMessage(Class _class, SEL sel, IMP imp, const char *prefix _default(NULL));
#endif
void MSHookMessageEx(Class _class, SEL sel, IMP imp, IMP *result);
#endif
#ifdef __cplusplus
}
#endif

template <typename Type_> Type_ &MSHookIvar(id self, const char *name) {
    Ivar ivar(class_getInstanceVariable(object_getClass(self), name));
    void *pointer(ivar == NULL ? NULL : reinterpret_cast<char *>(self) + ivar_getOffset(ivar));
    return *reinterpret_cast<Type_ *>(pointer);
}



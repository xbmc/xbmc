
#if @HAVE_VISIBILITY@ && BUILDING_LIBCHARSET
#define LIBCHARSET_DLL_EXPORTED __attribute__((__visibility__("default")))
#else
#define LIBCHARSET_DLL_EXPORTED
#endif


#if @HAVE_VISIBILITY@ && BUILDING_LIBICONV
#define LIBICONV_DLL_EXPORTED __attribute__((__visibility__("default")))
#else
#define LIBICONV_DLL_EXPORTED
#endif

#.rst:
# FindCCache
# ----------
# Finds ccache and sets it up as compiler wrapper.
# This should ideally be called before the call to project().
#
# See: https://crascit.com/2016/04/09/using-ccache-with-cmake/

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  # Supports Unix Makefiles and Ninja
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "${CCACHE_PROGRAM}")
endif()

# Check for intrinsics
# AC_MSG_CHECKING([for __sync_add_and_fetch(temp, 1)])
# AC_TRY_COMPILE([],[long* temp=0; __sync_add_and_fetch(temp, 1);],
#                 [have_builtin_sync_add_and_fetch=yes],
#                 [have_builtin_sync_add_and_fetch=no])
# AC_MSG_RESULT($have_builtin_sync_add_and_fetch)
# if test "x$have_builtin_sync_add_and_fetch" = "xyes"; then
#     AC_DEFINE(HAS_BUILTIN_SYNC_ADD_AND_FETCH, 1,
#         [Define to 1 if your compiler supports the __sync_add_and_fetch() intrinsic.])
# fi

# AC_MSG_CHECKING([for __sync_sub_and_fetch(temp, 1)])
# AC_TRY_COMPILE([],[long* temp=0; __sync_sub_and_fetch(temp, 1);],
#                 [have_builtin_sync_sub_and_fetch=yes],
#                 [have_builtin_sync_sub_and_fetch=no])
# AC_MSG_RESULT($have_builtin_sync_sub_and_fetch)
# if test "x$have_builtin_sync_sub_and_fetch" = "xyes"; then
#     AC_DEFINE(HAS_BUILTIN_SYNC_SUB_AND_FETCH, 1,
#         [Define to 1 if your compiler supports the __sync_sub_and_fetch() intrinsic.])
# fi

# AC_MSG_CHECKING([for __sync_val_compare_and_swap(temp, 1, 1)])
# AC_TRY_COMPILE([],[long *temp = 0; __sync_val_compare_and_swap(temp, 1, 1);],
#                 [have_builtin_sync_val_compare_and_swap=yes],
#                 [have_builtin_sync_val_compare_and_swap=no])
# AC_MSG_RESULT($have_builtin_sync_val_compare_and_swap)
# if test "x$have_builtin_sync_val_compare_and_swap" = "xyes"; then
#     AC_DEFINE(HAS_BUILTIN_SYNC_VAL_COMPARE_AND_SWAP, 1,
#         [Define to 1 if your compiler supports the __sync_val_compare_and_swap() intrinsic.])
# fi

CHECK_C_SOURCE_COMPILES("int main () { long* temp=0; __sync_add_and_fetch(temp, 1); }" HAS_BUILTIN_SYNC_ADD_AND_FETCH)
CHECK_C_SOURCE_COMPILES("int main() { long* temp=0; __sync_sub_and_fetch(temp, 1); }" HAS_BUILTIN_SYNC_SUB_AND_FETCH)
CHECK_C_SOURCE_COMPILES("int main() { long *temp = 0; __sync_val_compare_and_swap(temp, 1, 1); }" HAS_BUILTIN_SYNC_VAL_COMPARE_AND_SWAP)

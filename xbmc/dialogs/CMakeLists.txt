find_all_sources(. dialogs_SRCS)
list(REMOVE_ITEM dialogs_SRCS ./GUIDialogPlayerControls.cpp ./GUIDialogPlayerControls.h)
add_sources(${dialogs_SRCS})
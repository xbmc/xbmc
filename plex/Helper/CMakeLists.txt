aux_source_directory(. help_SRCS)
add_library(plex_helper STATIC ${help_SRCS})
add_dependencies(plex_helper git_revision.h)

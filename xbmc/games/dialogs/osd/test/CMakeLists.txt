set(SOURCES TestLeaderboardUtils.cpp
)

# Cleared explicitly: this directory is entered from the one above, which has
# already filled HEADERS with its own, and they would be looked for in here
set(HEADERS)

core_add_test_library(games_osd_dialogs_test)

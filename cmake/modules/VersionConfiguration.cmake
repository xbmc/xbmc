# Get the current date.
include(GetDate)
today(CURRENT_DATE)

# Get git revision version
include(GetGitRevisionDescription)
get_git_head_revision(REFSPEC FULL_GIT_REVISION)
if(FULL_GIT_REVISION STREQUAL "GITDIR-NOTFOUND")
  set(GIT_REVISION "git")
else(FULL_GIT_REVISION STREQUAL "GITDIR-NOTFOUND")
  string(SUBSTRING ${FULL_GIT_REVISION} 0 8 GIT_REVISION)
endif(FULL_GIT_REVISION STREQUAL "GITDIR-NOTFOUND")

# Get the build number if available
if(DEFINED ENV{BUILD_NUMBER})
  set(VERSION_BUILD "$ENV{BUILD_NUMBER}")
  set(VERSION_BUILD_NR "$ENV{BUILD_NUMBER}")
else()
  set(VERSION_BUILD "dev")
  set(VERSION_BUILD_NR "0")
endif()

set(VERSION_STRING "${GIT_REVISION}-${VERSION_BUILD}")
add_definitions(-DVERSION_STRING="${VERSION_STRING}" -DBUILD_DATE="${CURRENT_DATE}")

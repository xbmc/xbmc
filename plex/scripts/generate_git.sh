#!/bin/sh
GIT_REV=$(git --no-pager log --abbrev=7 -n 1 --pretty=format:"%h %ci" HEAD | awk '{gsub("-", "");print $2"-"$1}')
echo "#define GIT_REV \"$GIT_REV\"" > git_revision.h

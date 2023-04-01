#!/bin/bash

find xbmc -type f | xargs sed -i \
-e ':a
    /CLog::Log(/{
      :line
      s/,\n[ ]*/, /g
      s/(\n[ ]*/(/g
      s/"\s*\n\s*"/" "/g
      /);/!{
        N
        bline
      }
      N
    }'

git add xbmc
git commit --no-verify -m "[temp] convert clog calls from multiline to single line"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}: (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update loggings calls that only use __FUNCTION__ to use LogF"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}, (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update loggings calls that only use __FUNCTION__ and comma to use LogF"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} - (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update loggings calls that only use __FUNCTION__ and dash to use LogF"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update loggings calls that only use __FUNCTION__ without colon to use LogF"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), (LOG[A-Z]+), "\{\}: (.*)", __FUNCTION__\);/CLog::LogFC(\1, \2, "\3");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update component loggings calls that only use __FUNCTION__ to use LogF"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{0\} (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that only use __FUNCTION__ to use LogF (special case)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\}: (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2: \3");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that only use __FUNCTION__ to use LogF (with class)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\} - (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2: \3");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that only use __FUNCTION__ to use LogF (with class and dash)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\} (.*)", __FUNCTION__\);/CLog::LogF(\1, "\2: \3");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that only use __FUNCTION__ to use LogF (with class and no dash or colon)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\}", __FUNCTION__\);/CLog::LogF(\1, "\2");/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that only use __FUNCTION__ to use LogF (with class and no message)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}: (.*)", __FUNCTION__, ([^,]+)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and one extra arg to use LogF"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} - (.*)", __FUNCTION__, ([^,]+)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and one extra arg to use LogF (with dash)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} (.*)", __FUNCTION__, ([^,]+)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and one extra arg to use LogF (no dash or colon)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}, (.*)", __FUNCTION__, ([^,]+)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and one extra arg to use LogF (with comma)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}(.*)", __FUNCTION__, ([^,]+)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and one extra arg to use LogF (no space)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}: (.*)", __FUNCTION__, (.*)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and multiple args to use LogF"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} - (.*)", __FUNCTION__, (.*)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and multiple args to use LogF (with dash)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} (.*)", __FUNCTION__, (.*)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and multiple args to use LogF (no dash or colon)"

# find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}, (.*)", __FUNCTION__, (.*)\);/CLog::LogF(\1, "\2", \3);/g' -i

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}(.*)", __FUNCTION__, (.*)\);/CLog::LogF(\1, "\2", \3);/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and multiple args to use LogF (no space)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\}: (.*)", __FUNCTION__,$/CLog::LogF(\1, "\2: \3",/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and continue on newline to use LogF (with class)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\} - (.*)", __FUNCTION__,$/CLog::LogF(\1, "\2: \3",/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and continue on newline to use LogF (with class and dash)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}: (.*)", __FUNCTION__,$/CLog::LogF(\1, "\2",/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and continue on newline"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} - (.*)", __FUNCTION__,$/CLog::LogF(\1, "\2",/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and continue on newline (with dash)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\}, (.*)", __FUNCTION__,$/CLog::LogF(\1, "\2",/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and continue on newline (with comma)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "\{\} (.*)", __FUNCTION__,$/CLog::LogF(\1, "\2",/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and continue on newline (no dash or colon)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\}: (.*)", __FUNCTION__, (.*)/CLog::LogF(\1, "\2: \3", \4/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and multiple args to use LogF (with class)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\} - (.*)", __FUNCTION__, (.*)/CLog::LogF(\1, "\2: \3", \4/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and multiple args to use LogF (with class and dash)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), "(.*)::\{\} (.*)", __FUNCTION__, (.*)/CLog::LogF(\1, "\2: \3", \4/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ and multiple args to use LogF (with class and no dash or colon)"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), __FUNCTION__": /CLog::LogF(\1, "/g' -i
git add xbmc
git commit --no-verify -m "CLog: update logging calls that use __FUNCTION__ as macro"

find xbmc -type f | xargs sed -E -e 's/CLog::Log\((LOG[A-Z]+), __FUNCTION__\s*"\s*[:,-]\s*(.*)"/CLog::LogF(\1, "\2"/g' -i
git add xbmc
git commit --no-verify -m "CLog: update loggings calls that only use __FUNCTION__ to use LogF (with C string literal concat)"


#!/bin/bash
#
# Copyright 2009, 2010 10gen Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# $1 is C compiler. $2 is header file
cat <<EOF >tmp.c && $1 -o header_check.tmp tmp.c
#include <$2>
int main() { int64_t i=0; return 0; }
EOF

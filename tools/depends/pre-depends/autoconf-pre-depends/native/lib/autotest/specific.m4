# This file is part of Autoconf.                          -*- Autoconf -*-
# M4 macros used in running tests using third-party testing tools.
m4_define([_AT_COPYRIGHT_YEARS],
[Copyright (C) 2009, 2010 Free Software Foundation, Inc.])

# This file is part of Autoconf.  This program is free
# software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Under Section 7 of GPL version 3, you are granted additional
# permissions described in the Autoconf Configure Script Exception,
# version 3.0, as published by the Free Software Foundation.
#
# You should have received a copy of the GNU General Public License
# and a copy of the Autoconf Configure Script Exception along with
# this program; see the files COPYINGv3 and COPYING.EXCEPTION
# respectively.  If not, see <http://www.gnu.org/licenses/>.


## ------------------------ ##
## Erlang EUnit unit tests. ##
## ------------------------ ##

# AT_CHECK_EUNIT(MODULE, SPEC, [ERLFLAGS], [RUN-IF-FAIL], [RUN-IF-PASS])
# ----------------------------------------------------------------------
# Check that the EUnit test specification SPEC passes. The ERLFLAGS
# optional flags are passed to the Erlang interpreter command line to
# execute the test. The test is executed from an automatically
# generated Erlang module named MODULE. Each call to this macro should
# have a distinct MODULE name within each test group, to ease
# debugging.
# An Erlang/OTP version which contains the eunit library must be
# installed, in order to execute this macro in a test suite.  The ERL,
# ERLC, and ERLCFLAGS variables must be defined in atconfig,
# typically by using the AC_ERLANG_PATH_ERL and AC_ERLANG_PATH_ERLC
# Autoconf macros.
_AT_DEFINE_SETUP([AT_CHECK_EUNIT],
[AT_SKIP_IF([test ! -f "$ERL" || test ! -f "$ERLC"])
## A wrapper to EUnit, to exit the Erlang VM with the right exit code:
AT_DATA([$1.erl],
[[-module($1).
-export([test/0, test/1]).
test() -> test([]).
test(Options) ->
  TestSpec = $2,
  ReturnValue = case code:load_file(eunit) of
    {module, _} -> case eunit:test(TestSpec, Options) of
        ok -> "0\n"; %% test passes
        _  -> "1\n"  %% test fails
      end;
    _ -> "77\n" %% EUnit not found, test skipped
  end,
  file:write_file("$1.result", ReturnValue),
  init:stop().
]])
AT_CHECK(["$ERLC" $ERLCFLAGS -b beam $1.erl])
## Make EUnit verbose when testsuite is verbose:
if test -z "$at_verbose"; then
  at_eunit_options="verbose"
else
  at_eunit_options=""
fi
AT_CHECK(["$ERL" $3 -s $1 test $at_eunit_options -noshell], [0], [ignore], [],
         [$4], [$5])
AT_CAPTURE_FILE([$1.result])
AT_CHECK([test -f "$1.result" && (exit `cat "$1.result"`)])
])

@echo off
rem this gets the current branch from either the branchname (if we attached) or
rem by using scientific branch fetching algorithms [tm] git is in detached HEAD state
rem result will be in env var %BRANCH%
SET BRANCH=na
SET DETACHED=1
:: detect detached head
git symbolic-ref HEAD >nul 2>&1
IF %ERRORLEVEL%==0 (
  SET DETACHED=0
)
rem find the branchname - if current branch is a pr we have to take this into account aswell
rem (would be refs/pulls/pr/number/head then)
rem normal branch would be refs/heads/branchname
IF %DETACHED%==0 (
  FOR /f "tokens=3,4 delims=/" %%a IN ('git symbolic-ref HEAD') DO (
    IF %%a==pr (
      SET BRANCH=PR%%b
    ) ELSE (
      SET BRANCH=%%a
    )
  )
  GOTO branchfound
)

:: when building with jenkins there's no branch. First git command gets the branch even there
:: it ignores all branches in the pr/ scope (pull requests) and finds the first non pr branch
:: (this mimics what the linux side does via sed and head -n1
:: but is empty in a normal build environment. Second git command gets the branch there.
FOR /f "tokens=2,3 delims=/" %%a IN ('git branch -r --contains HEAD') DO (
  :: ignore pull requests
  IF NOT %%a==pr (
    rem our branch could be like origin/Frodo (we need %%a here)
    rem or our branch could be like origin/HEAD -> origin/master (we need %%b here)
    rem if we have %%b - use it - else use %%a
    IF NOT "%%b"=="" (
      :: we found the first non-pullrequest branch - gotcha
      SET BRANCH=%%b
    ) ELSE (
      SET BRANCH=%%a
    )
    GOTO branchfound
  )
)
IF "%BRANCH%"=="na" (
  FOR /f "tokens=* delims= " %%a IN ('git rev-parse --abbrev-ref HEAD') DO SET BRANCH=%%a
)
:branchfound

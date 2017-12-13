@echo off
rem this gets the current branch from either the branchname (if we attached) or
rem by using scientific branch fetching algorithms [tm] git is in detached HEAD state
rem result will be in env var %BRANCH%
SET BRANCH=
SET DETACHED=1
:: detect detached head
git symbolic-ref HEAD >nul 2>&1
IF %ERRORLEVEL%==0 (
  SET DETACHED=0
)
rem find the branchname - if current branch is a pr we have to take this into account aswell
rem (would be refs/heads/pr/number/head then)
rem normal branch would be refs/heads/branchname
IF %DETACHED%==0 (
  FOR /f %%a IN ('git symbolic-ref HEAD') DO SET BRANCH=%%a
  SETLOCAL EnableDelayedExpansion
  SET BRANCH=!BRANCH:*/=!
  SETLOCAL DisableDelayedExpansion
  GOTO branchfound
)

:: when building with jenkins there's no branch. First git command gets the branches
:: and then prefers a non pr branch if one exists
:: (this mimics what the linux side does via sed and head -n1
:: but is empty in a normal build environment. Second git command gets the branch there.
SET command=git branch -r --points-at HEAD
%command% >nul 2>&1
IF NOT %ERRORLEVEL%==0 (
  SET command=git branch -r --contains HEAD
)

%command% | findstr "%GITHUB_REPO%\/" | findstr /V "%GITHUB_REPO%\/pr\/" >nul
IF %ERRORLEVEL%==0 (
  FOR /f %%a IN ('%%command%% ^| findstr "%GITHUB_REPO%\/" ^| findstr /V "%GITHUB_REPO%\/pr\/"') DO (
      SET BRANCH=%%a
      GOTO branchfound
    )
) ELSE (
  FOR /f %%a IN ('%%command%% ^| findstr "%GITHUB_REPO%\/"') DO (
    SET BRANCH=%%a
    GOTO branchfound
  )
)

:branchfound
SETLOCAL EnableDelayedExpansion
IF NOT "!BRANCH!"=="" (
  :: BRANCH is now (head|GITHUB_REPO)/(branchname|pr/number/(head|merge))
  SET BRANCH=!BRANCH:/pr/=/PR!
  SET BRANCH=!BRANCH:*/=!
  SET BRANCH=!BRANCH:/=-!
)
SETLOCAL DisableDelayedExpansion

ECHO.%BRANCH%

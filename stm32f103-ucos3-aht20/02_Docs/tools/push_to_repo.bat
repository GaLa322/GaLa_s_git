@echo off
setlocal enabledelayedexpansion
title Push this project as a subfolder of an existing GitHub repo

rem ============================================================
rem  Purpose: add this STM32F103 + uC/OS-III + AHT20 project
rem           as a SUBFOLDER of an existing GitHub repository.
rem           Default target: https://github.com/GaLa322/GaLa_s_git
rem
rem  Usage:   1) turn on proxy/VPN (github.com is blocked on this machine)
rem           2) double-click this file
rem           optional: pass another repo url as the 1st argument, e.g.
rem              push_to_repo.bat https://github.com/you/your-repo.git
rem
rem  What it does:
rem    1. check github.com reachability
rem    2. clone your repo to ..\_push_work\GaLa_s_git  (your files untouched)
rem    3. export HEAD of this repo into <repo>\stm32f103-ucos3-aht20\
rem    4. git add + commit  (asks for name/email once if not configured)
rem    5. git push origin main
rem
rem  NOTE: keep this file pure ASCII. Chinese bytes inside a .bat break the
rem        cmd.exe parser, because GBK trail bytes can equal ^| ^" \ characters.
rem ============================================================

set "REPO=%~dp0..\.."
set "WORK=%~dp0..\..\..\_push_work"
set "REMOTE=https://github.com/GaLa322/GaLa_s_git.git"
if not "%~1"=="" set "REMOTE=%~1"
set "SUB=stm32f103-ucos3-aht20"
set "CLONE=%WORK%\GaLa_s_git"

echo.
echo ============================================================
echo   remote repo : %REMOTE%
echo   local repo  : %REPO%
echo   sub folder  : %SUB%\
echo ============================================================
echo.

echo [1/5] checking github.com ...
curl -s -o nul --max-time 12 https://github.com 2>nul
if errorlevel 1 (
    echo    [X] cannot reach github.com
    echo.
    echo    git push would fail. Start proxy/VPN, then run:
    echo        git config --global http.proxy  http://127.0.0.1:7890
    echo        git config --global https.proxy http://127.0.0.1:7890
    echo    ^(use your own port; Clash default is 7890^)
    echo.
    pause
    exit /b 1
)
echo    [OK]

echo [2/5] cloning ...
if not exist "%WORK%" mkdir "%WORK%"
if exist "%CLONE%" rmdir /s /q "%CLONE%"
git clone "%REMOTE%" "%CLONE%"
if errorlevel 1 (
    echo    [X] clone failed - check url, network and credentials
    pause
    exit /b 1
)
echo    [OK]

echo [3/5] exporting project files into %SUB%\ ...
if exist "%CLONE%\%SUB%" rmdir /s /q "%CLONE%\%SUB%"
rem local clone is the safest way to copy tracked files (tar.exe chokes on UTF-8 names)
git clone -q "%REPO%" "%CLONE%\%SUB%"
if errorlevel 1 (
    echo    [X] export failed - does the local repo have commits?
    pause
    exit /b 1
)
rmdir /s /q "%CLONE%\%SUB%\.git"
echo    [OK] project files copied

echo [4/5] committing ...
set "GNAME="
set "GMAIL="
for /f "delims=" %%i in ('git -C "%CLONE%" config user.name 2^>nul') do set "GNAME=%%i"
for /f "delims=" %%i in ('git -C "%CLONE%" config user.email 2^>nul') do set "GMAIL=%%i"
if "!GNAME!"=="" (
    set /p GNAME=    your name for git commits: 
    git -C "%CLONE%" config user.name "!GNAME!"
)
if "!GMAIL!"=="" (
    set /p GMAIL=    your GitHub email: 
    git -C "%CLONE%" config user.email "!GMAIL!"
)
echo     author: !GNAME! ^<!GMAIL!^>

git -C "%CLONE%" add -A
git -C "%CLONE%" commit -m "feat: add STM32F103 uC/OS-III + AHT20 project"
if errorlevel 1 echo    [!] nothing new to commit
echo    [OK]

echo [5/5] pushing to origin ...
echo       if Git Credential Manager pops up, choose "Sign in with your browser"
git -C "%CLONE%" push origin main
if errorlevel 1 (
    echo.
    echo    [X] push failed. usual causes:
    echo        - proxy not active ^(see step 1^)
    echo        - default branch is not main: run  git -C "%CLONE%" branch -a
    echo        - stale credentials: delete github.com entry in Windows Credential Manager
    pause
    exit /b 1
)

echo.
echo ============================================================
echo   DONE - open the repo page and look for %SUB%\
echo   project readme: %SUB%\README.md
echo   temp clone %CLONE% can be deleted
echo ============================================================
pause

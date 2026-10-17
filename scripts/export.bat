@echo off
rem export.bat — package a project for delivery (Windows)
rem Usage: .\scripts\export.bat <project_name> <archive_name>
rem Example: .\scripts\export.bat I Camera_delivery

setlocal EnableExtensions EnableDelayedExpansion

if "%~1"=="" goto :usage
if "%~2"=="" goto :usage

set "PROJECT=%~1"
set "ARCHIVE=%~2"
if /i "!ARCHIVE:~-4!"==".zip" set "ARCHIVE=!ARCHIVE:~0,-4!"

for %%I in ("%~dp0..") do set "ROOT=%%~fI"
set "PROJECT_DIR=%ROOT%\%PROJECT%"
set "TMP=%ROOT%\scripts\outputs\tmp_%PROJECT%"
set "OUT=%ROOT%\scripts\outputs\%ARCHIVE%.zip"
set "SRC_DIR=%PROJECT_DIR%\src"

if not exist "%PROJECT_DIR%" (
    echo ERROR: project directory not found: "%PROJECT_DIR%" >&2
    exit /b 1
)

rem Clean any previous tmp run
if exist "%TMP%" rd /s /q "%TMP%"
if not exist "%ROOT%\scripts\outputs" md "%ROOT%\scripts\outputs"
if exist "%OUT%" del /f /q "%OUT%" >nul 2>&1
md "%TMP%\screenshot"
md "%TMP%\video"
md "%TMP%\eseguibili"
md "%TMP%\src"

rem 1. src/ — filtered by .deliverignore (root and/or project-level)
set "EXCLUDE_DIRS="
set "EXCLUDE_FILES="
set "HAS_IGNORE=0"

if exist "%ROOT%\.deliverignore" (
    set "HAS_IGNORE=1"
    call :parse_ignore "%ROOT%\.deliverignore"
)
if exist "%PROJECT_DIR%\.deliverignore" (
    set "HAS_IGNORE=1"
    call :parse_ignore "%PROJECT_DIR%\.deliverignore"
)

if "!HAS_IGNORE!"=="0" (
    echo WARNING: no .deliverignore found at "%ROOT%\.deliverignore" or "%PROJECT_DIR%\.deliverignore" — copying src\ unfiltered
)

set "ROBO_OPTS=/E /NJH /NJS /NDL /NC /NS"
if defined EXCLUDE_DIRS set "ROBO_OPTS=!ROBO_OPTS! /XD !EXCLUDE_DIRS!"
if defined EXCLUDE_FILES set "ROBO_OPTS=!ROBO_OPTS! /XF !EXCLUDE_FILES!"

robocopy "%SRC_DIR%" "%TMP%\src" !ROBO_OPTS! >nul
if %ERRORLEVEL% GEQ 8 (
    echo ERROR: robocopy failed with exit code %ERRORLEVEL% >&2
    exit /b 1
)

rem 2. deliverables: images -> screenshot/
for %%E in (bmp jpg jpeg png gif tiff webp) do (
    if exist "%PROJECT_DIR%\deliverables\*.%%E" (
        copy /y "%PROJECT_DIR%\deliverables\*.%%E" "%TMP%\screenshot\" >nul 2>&1
    )
)

rem 3. deliverables: video -> video/
for %%E in (mp4 mov mpeg avi mkv) do (
    if exist "%PROJECT_DIR%\deliverables\*.%%E" (
        copy /y "%PROJECT_DIR%\deliverables\*.%%E" "%TMP%\video\" >nul 2>&1
    )
)

rem 4. deliverables: html + README.txt -> root
if exist "%PROJECT_DIR%\deliverables\*.html" (
    copy /y "%PROJECT_DIR%\deliverables\*.html" "%TMP%\" >nul 2>&1
)
if exist "%PROJECT_DIR%\deliverables\README.txt" (
    copy /y "%PROJECT_DIR%\deliverables\README.txt" "%TMP%\" >nul 2>&1
)

rem 5. eseguibili — Release/x64 of the .sln found in src/, or fallback to cmake-build-release
set "SLN="
for %%F in ("%SRC_DIR%\*.sln") do (
    if not defined SLN set "SLN=%%~nF"
)
if not defined SLN (
    echo ERROR: no .sln file found in "%SRC_DIR%" >&2
    exit /b 1
)

set "EXEC_SRC=%SRC_DIR%\!SLN!\Release\x64"
set "CMAKE_SRC=%SRC_DIR%\cmake-build-release"
if not exist "%CMAKE_SRC%" set "CMAKE_SRC=%PROJECT_DIR%\cmake-build-release"

if exist "%EXEC_SRC%" (
    robocopy "%EXEC_SRC%" "%TMP%\eseguibili" /E /NJH /NJS /NDL /NC /NS >nul 2>&1
) else if exist "%CMAKE_SRC%" (
    for %%E in (exe dll) do (
        if exist "%CMAKE_SRC%\*.%%E" (
            copy /y "%CMAKE_SRC%\*.%%E" "%TMP%\eseguibili\" >nul 2>&1
        )
    )
) else (
    echo WARNING: neither "%EXEC_SRC%" nor "%CMAKE_SRC%" found — eseguibili\ will be empty
)

rem 6. zip
where tar >nul 2>&1
if %ERRORLEVEL% equ 0 (
    tar -a -c -f "%OUT%" -C "%TMP%" *
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -Path '%TMP%\*' -DestinationPath '%OUT%' -Force"
)

if not exist "%OUT%" (
    echo ERROR: failed to create archive "%OUT%" >&2
    exit /b 1
)

rem 7. cleanup
rd /s /q "%TMP%"

echo Done: "%OUT%"
exit /b 0

:parse_ignore
for /f "usebackq eol=# tokens=*" %%L in ("%~1") do (
    set "ENTRY=%%L"
    if "!ENTRY!" neq "" (
        if "!ENTRY:~-1!"=="/" (
            set "D=!ENTRY:~0,-1!"
            set "EXCLUDE_DIRS=!EXCLUDE_DIRS! "!D!""
        ) else if "!ENTRY:~-1!"=="\" (
            set "D=!ENTRY:~0,-1!"
            set "EXCLUDE_DIRS=!EXCLUDE_DIRS! "!D!""
        ) else (
            set "EXCLUDE_FILES=!EXCLUDE_FILES! "!ENTRY!""
            set "EXCLUDE_DIRS=!EXCLUDE_DIRS! "!ENTRY!""
        )
    )
)
exit /b 0

:usage
echo Usage: %~nx0 ^<project_name^> ^<archive_name^> >&2
echo Example: %~nx0 I Camera_delivery >&2
exit /b 1


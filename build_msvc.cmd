@echo off
setlocal EnableExtensions

:: 1. DETECT VISUAL STUDIO (with parameter hint)
:: =====================================================================
:: Usage (without parameter defaults to VS2022)
:: build_msvc.bat 2022
:: build_msvc.bat 2026
:: build_msvc.bat 2026v143

set "HINT=%1"
set "VS_TOOLSET="

set "VS2022_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
set "VS2026_PATH=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"

if "%HINT%"=="2026v143" goto :try_2026v143
if "%HINT%"=="2026" goto :try_2026
if "%HINT%"=="2022" goto :try_2022

:try_2022
if exist "%VS2022_PATH%" (
    set "VCVARS=%VS2022_PATH%"
    set "VS_GEN=Visual Studio 17 2022"
    echo [INFO] Selected Visual Studio 2022
    goto :found_vs
)

:try_2026
if exist "%VS2026_PATH%" (
    set "VCVARS=%VS2026_PATH%"
    set "VS_GEN=Visual Studio 18 2026"
    echo [INFO] Selected Visual Studio 2026
    goto :found_vs
)

:try_2026v143
if exist "%VS2026_PATH%" (
    set "VCVARS=%VS2026_PATH%"
    set "VS_GEN=Visual Studio 18 2026"
    set "VS_TOOLSET=v143"
    echo [INFO] Selected Visual Studio 2026 with v143 Toolset (14.44)
    goto :found_vs
)

if not defined VCVARS (
    echo [ERROR] vcvarsall.bat not found at any known location.
    pause
    exit /b 1
)

:found_vs
set "SAFE_PATH=C:\Windows\System32;C:\Windows;C:\Program Files\Meson;%LocalAppData%\Microsoft\WinGet\Links"

:: 2. BUILD 64-BIT (x64)
echo ============================================================
echo BUILDING 64-BIT (x64)
echo ============================================================
call :run_build x64 C:/lib64 build/x64
if %ERRORLEVEL% NEQ 0 goto :failed

:: 3. BUILD 32-BIT (Win32)
echo ============================================================
echo BUILDING 32-BIT (Win32)
echo ============================================================
call :run_build x86 C:/lib32 build/Win32
if %ERRORLEVEL% NEQ 0 goto :failed

echo.
echo SUCCESS: All builds completed.
pause
exit /b 0

:failed
echo [ERROR] Build failed.
pause
exit /b 1

:: =====================================================================
:: BUILD ROUTINE (Using a sub-process to isolate environment)
:: =====================================================================
:run_build
:: %1 = arch (x64/x86), %2 = prefix (C:/lib64), %3 = build_dir (build/x64)
set "ARCH=%1"
set "PREFIX=%2"
set "BDIR=%3"

set "TFILE=run_temp_%ARCH%.bat"

set "CMAKE_TS="
set "MSB_TS="
if defined VS_TOOLSET (
    set "CMAKE_TS=-T %VS_TOOLSET%"
    set "MSB_TS=/p:PlatformToolset=%VS_TOOLSET%"
)

:: We create a small temporary script to ensure CALL and PATH work perfectly
echo @echo off > "%TFILE%"
echo set "PATH=%SAFE_PATH%" >> "%TFILE%"

if defined VS_TOOLSET (
:: When defined, we want v143.
:: using the latest known v143 compatible compiler version (19.44.35221 is found as of January 2026)
:: {none} for latest installed VC++ compiler toolset
:: "14.0" for VC++ 2015, "14.xx" for the latest 14.xx.yyyyy toolset installed (e.g. "14.11")
:: "14.xx.yyyyy" for a specific full version number (e.g. "14.11.25503")
    echo call "%VCVARS%" %ARCH% -vcvars_ver=14.44 >> "%TFILE%"
) else (
    echo call "%VCVARS%" %ARCH% >> "%TFILE%"
)

echo set "PATH=%SAFE_PATH%;%%PATH%%" >> "%TFILE%"
:: check folder before we enter
echo if not exist "libass" ( echo [ERROR] libass folder not found! ^& exit /b 1 ) >> "%TFILE%"
echo cd libass >> "%TFILE%"
echo set CC=cl >> "%TFILE%"
echo set CXX=cl >> "%TFILE%"
echo meson setup %BDIR% --wrap-mode=forcefallback -Ddefault_library=static -Dbuildtype=release -Dasm=enabled -Db_vscrt=static_from_buildtype --prefix=%PREFIX% --wipe ^|^| exit /b 1 >> "%TFILE%"
echo meson compile -C %BDIR% ^|^| exit /b 1 >> "%TFILE%"
echo meson install -C %BDIR% ^|^| exit /b 1 >> "%TFILE%"
echo cd .. >> "%TFILE%"
echo set "PKG_CONFIG_PATH=%PREFIX%/lib/pkgconfig" >> "%TFILE%"
echo if exist "%BDIR%\CMakeCache.txt" del "%BDIR%\CMakeCache.txt" >> "%TFILE%"
echo if exist "%BDIR%\CMakeFiles" rmdir /S /Q "%BDIR%\CMakeFiles" >> "%TFILE%"
echo cmake -G "%VS_GEN%" %CMAKE_TS% -A %ARCH:x86=Win32% -D CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded -S . -B %BDIR% ^|^| exit /b 1 >> "%TFILE%"
echo msbuild /t:Rebuild /m /p:Configuration=Release %MSB_TS% /p:Platform=%ARCH:x86=Win32% .\%BDIR%\assrender.sln ^|^| exit /b 1 >> "%TFILE%"

call "%TFILE%"
set "RES=%ERRORLEVEL%"
if exist "%TFILE%" del "%TFILE%"
exit /b %RES%


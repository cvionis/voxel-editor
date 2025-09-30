@echo off
setlocal

:: --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ::
:: Note: This script assumes that it will be called from project's root directory. :: 
:: --- --- --- --- --- --- --- --- --- --- --- -- ---- -- --- --- --- --- ---- --- ::

:: --- Build from an environment where MSVC compiler tools exist ------------
where /q cl || (
  call C:\"Program Files"\"Microsoft Visual Studio"\2022\Community\VC\Auxiliary\Build\vcvarsall.bat x64 > nul 
)

:: --- Prepare arguments ----------------------------------------------------
set "release=0"
set "asan=0"
for %%a in (%*) do set "%%a=1"

:: --- Prepare build directory ---------------------------------------------
set root=%CD%
if not exist %root%\build mkdir %root%\build

:: --- Determine build configuration ---------------------------------------
set debug=
set cl_optimize_flags=

if "%release%"=="1" (
  set debug=0 
  set cl_optimize_flags=/O2 
  echo [release]
) else (
  set debug=1
  set cl_optimize_flags=/Od
  echo [debug]
)

:: --- Compiler flags -----------------------------------------------------
set exe_name=editor.exe

set cl_build_flags=/DR_BACKEND_GL=1 /DBUILD_CLI=0 /DBUILD_DEBUG=%debug%
set cl_warning_flags=/D_CRT_SECURE_NO_WARNINGS /wd4201 /wd4456 /wd4505 /W4
set cl_common=/Fe:%exe_name% /nologo /FC /Zi /diagnostics:caret /std:c++17 

if "%asan%"=="1" ( 
  set cl_common=%cl_common% /fsanitize=address 
  echo [asan enabled]
)

set compiler_flags=%cl_common% %cl_optimize_flags% %cl_build_flags% %cl_warning_flags%

:: --- Includes -----------------------------------------------------------
set freetype="%root%\src\third_party\freetype-2.11.1"

set includes=
set includes=%includes% /FI %freetype%\include\ft2build.h 
set includes=%includes% /I %freetype%\include 
set includes=%includes% /I ..\src\third_party\opengl 
set includes=%includes% /I ..\src\third_party\stb 

:: -- Linker flags --------------------------------------------------------
set linker_flags=/link %freetype%\build\freetype.lib /ignore:4099 /INCREMENTAL:no

:: --- Build targets ------------------------------------------------------
set targets=
set targets=%targets% %root%\src\main.cpp

:: --- Compile and link the program  --------------------------------------
pushd %root%\build
cl.exe %compiler_flags% %includes% %targets% %linker_flags%
,popd 

:: --- Unset variables ----------------------------------------------------
for %%a in (%*) do set "%%a=0"
endlocal

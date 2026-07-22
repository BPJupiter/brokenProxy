@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"
:restart

:: --- Usage Notes (2024/1/10) ------------------------------------------------
::
:: This is a central build script for the RAD Debugger project, for use in
:: Windows development environments. It takes a list of simple alphanumeric-
:: only arguments which control (a) what is built, (b) which compiler & linker
:: are used, and (c) extra high-level build options. By default, if no options
:: are passed, then the main "raddbg" graphical debugger is built.
::
:: Below is a non-exhaustive list of possible ways to use the script:
:: `build raddbg`
:: `build raddbg clang`
:: `build raddbg release`
:: `build raddbg asan telemetry`
:: `build rdi_from_pdb`
::
:: For a full list of possible build targets and their build command lines,
:: search for @build_targets in this file.
::
:: Below is a list of all possible non-target command line options:
::
:: - `asan`: enable address sanitizer
:: - `ubsan`: enable undefined-behavior sanitizer
:: - `telemetry`: enable RAD telemetry profiling support
:: - `spall`: enable spall profiling support

:: --- Unpack Arguments -------------------------------------------------------
for %%a in (%*) do set "%%~a=1"
if not "%msvc%"=="1" if not "%clang%"=="1" set msvc=1
if not "%release%"=="1" set debug=1
if "%debug%"=="1"   set release=0 && echo [debug mode]
if "%release%"=="1" set debug=0 && echo [release mode]
if "%msvc%"=="1"    set clang=0 && echo [msvc compile]
if "%clang%"=="1"   set msvc=0 && echo [clang compile]
if "%~1"==""                     echo [default mode, assuming `raddbg` build] && set raddbg=1&& set com_shim=1&& set raddbg_com_shim=1
if "%~1"=="release" if "%~2"=="" echo [default mode, assuming `raddbg` build] && set raddbg=1&& set com_shim=1&& set raddbg_com_shim=1

:: --- Unpack Command Line Build Arguments ------------------------------------
set auto_compile_flags=
if "%telemetry%"=="1"               set auto_compile_flags=%auto_compile_flags% -DPROFILE_TELEMETRY=1 && echo [telemetry profiling enabled]
if "%spall%"=="1"                   set auto_compile_flags=%auto_compile_flags% -DPROFILE_SPALL=1 && echo [spall profiling enabled]
if "%asan%"=="1"                    set auto_compile_flags=%auto_compile_flags% -fsanitize=address && echo [asan enabled]
if "%ubsan%"=="1"                   set auto_compile_flags=%auto_compile_flags% -fsanitize=undefined && echo [ubsan enabled]
if "%opengl%"=="1"                  set auto_compile_flags=%auto_compile_flags% -DR_BACKEND=R_BACKEND_OPENGL && echo [opengl render backend]
if "%dwarf%"=="1" if "%clang%"=="1" set auto_compile_flags=%auto_compile_flags% -gdwarf && echo [dwarf debug info]
if "%dwarf%"==""  if "%clang%"=="1" set auto_compile_flags=%auto_compile_flags% -gcodeview
if "%pgo%"=="1" (
  where llvm-profdata /q || echo llvm-profdata is not in the PATH || exit /b 1 
  if "%clang%"=="1" (
    if "%pgo_run%" == "1" (
      call llvm-profdata merge %LLVM_PROFILE_FILE% -output=%~dp0build\build.profdata || exit /b 1
      set auto_compile_flags=%auto_compile_flags% -fprofile-use=%~dp0build\build.profdata
      set pgo_run=0
    ) else (
      echo [pgo enabled]
      set auto_compile_flags=%auto_compile_flags% -fprofile-generate -mllvm -vp-counters-per-site=5
      set LLVM_PROFILE_FILE=%~dp0build\build.profraw
      set pgo_run=1
    )
  ) else (
    echo ERROR: PGO build is not supported with current compiler
    exit /b 1
  )
)

:: --- Compile/Link Line Definitions ------------------------------------------
set cl_common=     /I..\src\ /I..\local\ /nologo /FC /Z7 /Zc:preprocessor
set cl_debug=      call cl /Od /Ob1 /DBUILD_DEBUG=1 %cl_common% %auto_compile_flags% 
set cl_release=    call cl /O2 /DBUILD_DEBUG=0 %cl_common% %auto_compile_flags%
set cl_link=       /link /MANIFEST:EMBED /INCREMENTAL:NO /pdbaltpath:%%%%_PDB%%%% /noexp /nocoffgrpinfo /opt:ref /opt:icf
set cl_out=        /out:
set cl_obj_out=    /Fo:
set cl_linker=     
set clang_common=  -mcx16 -I..\src\ -I..\local\ -fdiagnostics-absolute-paths -Wall -Wno-unknown-warning-option -Wno-missing-braces -Wno-unused-function -Wno-unused-parameter -Wno-writable-strings -Wno-missing-field-initializers -Wno-unused-value -Wno-unused-variable -Wno-unused-local-typedef -Wno-deprecated-register -Wno-deprecated-declarations -Wno-unused-but-set-variable -Wno-single-bit-bitfield-constant-conversion -Wno-compare-distinct-pointer-types -Wno-initializer-overrides -Wno-incompatible-pointer-types-discards-qualifiers -Xclang -flto-visibility-public-std -D_USE_MATH_DEFINES -Dstrdup=_strdup -Dgnu_printf=printf -ferror-limit=10000
set clang_debug=   call clang -g -O0 -DBUILD_DEBUG=1 -D_DEBUG %clang_common% %auto_compile_flags%
set clang_release= call clang -g -O2 -DBUILD_DEBUG=0 -DNDEBUG %clang_common% %auto_compile_flags%
set clang_link=    -fuse-ld=lld -Xlinker /MANIFEST:EMBED -Xlinker /pdbaltpath:%%%%_PDB%%%% -Xlinker /NATVIS:"%~dp0\src\natvis\base.natvis" -Xlinker /opt:ref -Xlinker /opt:noicf
set clang_out=     -o
set clang_obj_out= -o
set clang_linker=  -Xlinker

:: --- Per-Build Settings -----------------------------------------------------
set link_icon=logo.res
if "%msvc%"=="1"    set linker=%cl_linker%
if "%clang%"=="1"   set linker=%clang_linker%
if "%msvc%"=="1"    set only_compile=/c
if "%clang%"=="1"   set only_compile=-c
if "%msvc%"=="1"    set EHsc=/EHsc
if "%clang%"=="1"   set EHsc=
if "%msvc%"=="1"    set no_aslr=/DYNAMICBASE:NO
if "%clang%"=="1"   set no_aslr=-Wl,/DYNAMICBASE:NO
if "%msvc%"=="1"    set rc=call rc
if "%clang%"=="1"   set rc=call llvm-rc
if "%msvc%"=="1"    set link_dll=/link /DLL
if "%clang%"=="1"   set link_dll=-Xlinker -DLL

:: --- Choose Compile/Link Lines ----------------------------------------------
if "%msvc%"=="1"      set compile_debug=%cl_debug%
if "%msvc%"=="1"      set compile_release=%cl_release%
if "%msvc%"=="1"      set compile_link=%cl_link%
if "%msvc%"=="1"      set out=%cl_out%
if "%msvc%"=="1"      set obj_out=%cl_obj_out%
if "%clang%"=="1"     set compile_debug=%clang_debug%
if "%clang%"=="1"     set compile_release=%clang_release%
if "%clang%"=="1"     set compile_link=%clang_link%
if "%clang%"=="1"     set out=%clang_out%
if "%clang%"=="1"     set obj_out=%clang_obj_out%
if "%debug%"=="1"     set compile=%compile_debug%
if "%release%"=="1"   set compile=%compile_release%

:: --- Prep Directories -------------------------------------------------------
if not exist build mkdir build
if not exist local mkdir local

:: --- Get Current Git Commit Id ----------------------------------------------
for /f %%i in ('call git describe --always --dirty')   do set compile=%compile% -DBUILD_GIT_HASH=\"%%i\"
for /f %%i in ('call git rev-parse HEAD')              do set compile=%compile% -DBUILD_GIT_HASH_FULL=\"%%i\"

:: --- Build Everything (@build_targets) --------------------------------------
pushd build
if "%torment%"=="1"                    set didbuild=1 && %compile% ..\src\torment\torment_main.c                             %compile_link% %out%torment.exe || exit /b1
popd

:: --- Warn On No Builds ------------------------------------------------------
if "%didbuild%"=="" (
  echo [WARNING] no valid build target specified; must use build target names as arguments to this script, like `build raddbg` or `build rdi_from_pdb`.
  exit /b 1
)


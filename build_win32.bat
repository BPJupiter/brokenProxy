@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set OUT_DIR=Debug
@set OUT_EXE=BrokenProxy
@set INCLUDES=/I. ^
              /Ithird_party\imgui\include ^
              /Ithird_party\portable_file_dialogs\include ^
              /Ithird_party\nlohmann\include ^
              /Ithird_party\nanosvg\include ^
              /Ithird_party\mapbox\include ^
              /Ithird_party\wepoll\include ^
              /Isrc\shared ^
              /Isrc\proxy ^
              /Ilib
@set SOURCES=third_party\imgui\src\imgui*.cpp ^
             third_party\wepoll\src\wepoll.c ^
             src\shared\Clay\*.c ^
             src\shared\Europa\*.c ^
             src\shared\Styx\*.c ^
             src\shared\Talos\*.c ^
             src\proxy\bproxy.c ^
             src\proxy\dns_resolution.c ^
             src\proxy\network_tools.c ^
             src\main.cpp
@set LIBS=lib\SDL3\SDL3.lib
mkdir %OUT_DIR%
cl /fsanitize=address /nologo /Zi /EHsc /MD /utf-8 %INCLUDES% %SOURCES% /Fd%OUT_DIR%/ /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link /CETCOMPAT %LIBS% /subsystem:console
copy /y lib\SDL3\SDL3.dll %OUT_DIR%\
xcopy /y /s /e /i assets %OUT_DIR%\assets

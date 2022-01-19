@echo off
REM i run this from root dir

ctime -begin misc\su.ctm

pushd src

REM _____________________________________________________________________________________________________
REM                                       Includes/Sources/Libs
REM _____________________________________________________________________________________________________

@set INCLUDES=/I"..\src" /I"C:\apps\graphviz\include"
@set SOURCES=su-main.cpp
@set LIBS=/libpath:C:\apps\graphviz\lib cdt.lib cgraph.lib gvc.lib user32.lib gdi32.lib winmm.lib

REM _____________________________________________________________________________________________________
REM                                      Compiler and Linker Flags
REM _____________________________________________________________________________________________________

@set WARNINGS=/W1 /wd4201 /wd4100 /wd4189 /wd4706 /wd4311 /wd4552
@set COMPILE_FLAGS=/diagnostics:column /EHsc /nologo /MT /MP /Oi /GR /Gm- /Fm /std:c++17 %WARNINGS%
@set LINK_FLAGS=/nologo /opt:ref /incremental:no
@set OUT_EXE=su.exe

REM _____________________________________________________________________________________________________
REM                                            Defines
REM _____________________________________________________________________________________________________

REM  SU_SLOW:     slow code allowed (Assert, etc)
REM  SU_INTERNAL: build for developer only

@set DEFINES_DEBUG=/D"SU_INTERNAL=1" /D"SU_SLOW=1" /D"_CRT_SECURE_NO_WARNINGS"
@set DEFINES_RELEASE=

REM _____________________________________________________________________________________________________
REM                                    Command Line Arguments
REM _____________________________________________________________________________________________________

IF [%1]==[] GOTO DEBUG
IF [%1]==[-i] GOTO ONE_FILE
IF [%1]==[-l] GOTO LINK_ONLY
IF [%1]==[-r] GOTO RELEASE

REM _____________________________________________________________________________________________________
REM                              DEBUG (compiles without optimization)
REM _____________________________________________________________________________________________________

:DEBUG
ECHO %DATE% %TIME%    Debug
ECHO ---------------------------------
@set OUT_DIR="..\build\debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /Z7 /Od %COMPILE_FLAGS% %DEFINES_DEBUG% %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LINK_FLAGS% %LIBS%
GOTO DONE

REM _____________________________________________________________________________________________________
REM    ONE FILE (compiles just one file with debug options, links with previously created .obj files)
REM _____________________________________________________________________________________________________

:ONE_FILE
ECHO %DATE% %TIME%    One File (Debug)
ECHO ---------------------------------
IF [%~2]==[] ECHO "Place the .cpp path after using -i"; GOTO DONE;
ECHO [93mWarning: debugging might not work with one-file compilation[0m

@set OUT_DIR="..\build\debug"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /c /Z7 %COMPILE_FLAGS% %DEFINES_DEBUG% %INCLUDES% %~2 /Fo%OUT_DIR%/
pushd ..\build\Debug
link %LINK_FLAGS% *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM _____________________________________________________________________________________________________
REM                           LINK ONLY (links with previously created .obj files)
REM _____________________________________________________________________________________________________

:LINK_ONLY
ECHO %DATE% %TIME%    Link Only (Debug)
ECHO ---------------------------------
pushd ..\build\Debug
link %LINK_FLAGS% *.obj %LIBS% /OUT:%OUT_EXE% 
popd
GOTO DONE

REM _____________________________________________________________________________________________________
REM                                 RELEASE (compiles with optimization)
REM _____________________________________________________________________________________________________

:RELEASE
ECHO %DATE% %TIME%    Release
@set OUT_DIR="..\build\release"
IF NOT EXIST %OUT_DIR% mkdir %OUT_DIR%
cl /O2 %COMPILE_FLAGS% %DEFINES_RELEASE% %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE% /Fo%OUT_DIR%/ /link %LINK_FLAGS% %LIBS%
GOTO DONE

:DONE
ECHO ---------------------------------
popd

ctime -end misc\su.ctm
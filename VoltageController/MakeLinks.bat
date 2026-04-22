@echo off

set ProjectDir=%~dp0
set Lib=%ProjectDir%lib\

set Libraries=%USERPROFILE%\Documents\Arduino\Libraries\

rmdir /S/Q %Lib% 2>NUL
mkdir %Lib%

::============================================
:: Create links
::============================================

call :Link_LIB              NYG

goto :END

:END

pause

goto :eof


::---------------------------
:Link_LIB
::---------------------------

if exist %Lib%%1        rmdir  /s/q %Lib%%1

set SOURCE_DIR=%1
if not "%2"=="."    (
    set SOURCE_DIR=%SOURCE_DIR%\%2
    )

mklink /J %Lib%%1       %Libraries%%SOURCE_DIR%

goto :eof

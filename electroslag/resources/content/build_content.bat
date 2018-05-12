@ECHO OFF
REM Build content files using an already existing electroslag_nores.exe whose path
REM is passed into this batch file as %1. The content json files are located relative
REM to this bat file's path.
IF "%~1"=="" GOTO no_electroslag
IF NOT EXIST "%~1" GOTO no_electroslag

PUSHD "%~dp1"

REM Cleanup old content builds
IF EXIST loading_screen.bin DEL loading_screen.bin
IF EXIST content.bin DEL content.bin

ECHO    Building content
electroslag_nores.exe --opt
IF %ERRORLEVEL% NEQ 0 GOTO electroslag_error

REM Remember to update the mask values if resource ids change!
ECHO    Updating electroslag.exe resources with content
ResourceHacker.exe -open electroslag_nores.exe -save electroslag.exe -action add -res loading_screen.bin -mask 512,9001,
IF %ERRORLEVEL% NEQ 0 GOTO resourcehacker_error

POPD
EXIT /B 0

:no_electroslag
ECHO Content build error: Could not find electroslag_nores program.
EXIT /B 1

:electroslag_error
ECHO Content build error: Electroslag opt failed.
POPD
EXIT /B 2

:resourcehacker_error
ECHO Content build error: ResourceHacker failed.
POPD
EXIT /B 3

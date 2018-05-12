@ECHO OFF
REM Build electroslag.exe from electroslag_nores.exe (given as %1) and a path to
REM the content files (%2.)
IF "%~1"=="" GOTO no_electroslag
IF NOT EXIST "%~1" GOTO no_electroslag

IF "%~2"=="" GOTO no_content
IF NOT EXIST "%~2" GOTO no_content

REM Remember to update the mask values if resource ids change!
ECHO    Updating electroslag.exe resources with content
ResourceHacker.exe -open "%~1" -save "%~dp1\electroslag.exe" -action add -res "%~2\loading_screen.bin" -mask 512,9001,
IF %ERRORLEVEL% NEQ 0 GOTO resourcehacker_error

EXIT /B 0

:no_electroslag
ECHO Content build error: Could not find electroslag_nores program.
EXIT /B 1

:no_content
ECHO Content build error: Could not find content.
EXIT /B 2

:resourcehacker_error
ECHO Content build error: ResourceHacker failed.
EXIT /B 3

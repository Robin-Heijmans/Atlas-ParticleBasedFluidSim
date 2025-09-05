
REM @echo off
call "%~dp0\vars.bat"

call "%UE5_BUILDTOOL_EXE%" -projectfiles -project="%UPROJECT_PATH%" -game -rocket -progress -log="%ROOTDIR%/Saved/Logs/UnrealVersionSelector-2022.07.24-13.51.06.log"

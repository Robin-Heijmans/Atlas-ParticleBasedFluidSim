
REM @echo off

call "%~dp0\vars.bat"
call "%UE5_EDITOR_EXE%" "%UPROJECT_PATH%" -log

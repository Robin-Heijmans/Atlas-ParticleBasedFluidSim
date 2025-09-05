
REM @echo off

call "%~dp0\vars.bat"
call "%UE5_EDITOR_CMD_EXE%" "%UPROJECT_PATH%" -run=cook -targetplatform=Windows

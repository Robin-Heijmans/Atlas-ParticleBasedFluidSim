
REM @echo off

call "%~dp0\vars.bat"
call "%UE5_EDITOR_EXE%" "%UPROJECT_PATH%" -game -log -windowed -resx=1280 -resy=720

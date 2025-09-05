
REM @echo off

call "%~dp0\vars.bat"
call "%PROJECT_BIN_DIR%"\%PROJECT%.exe -log -windowed -resx=1280 -resy=720 
    
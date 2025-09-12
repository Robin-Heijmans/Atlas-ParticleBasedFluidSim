call echo generating vars.bat
(
    echo REM @echo off
    echo set UE5_DIR=C:\Program Files\Epic Games\UE_5.5
    echo set ROOTDIR=%~dp0
    echo set ROOTDIR=%%ROOTDIR:~0,-1%%
    echo set PROJECT=fluid_simulation
    echo set PROJECT_DIR=%%ROOTDIR%%
    echo set PROJECT_BIN_DIR=%%ROOTDIR%%\Binaries\Win64
    echo set UPROJECT_PATH=%%PROJECT_DIR%%\%%PROJECT%%.uproject
    echo set UE5_EDITOR_EXE=%%UE5_DIR%%\Engine\Binaries\Win64\UnrealEditor.exe
    echo set UE5_EDITOR_CMD_EXE=%%UE5_DIR%%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
    echo set UE5_BUILDTOOL_EXE=%%UE5_DIR%%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe
    echo set BUILD_BAT=%%UE5_DIR%%\Engine\Build\BatchFiles\Build.bat
)>vars.bat

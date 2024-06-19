@echo off

SET "vcvarsall=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

IF EXIST "%vcvarsall%" (
CALL "%vcvarsall%" x64
)

REM set VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
REM set VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_api_dump;VK_LAYER_KHRONOS_validation

SET "path=W:\Warpunk;%path%"

START /MAX "" W:\4coder\4ed.exe
START /MAX "" W:\remedybg\remedybg.exe
START /MAX "" "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe" "W:\Warpunk"
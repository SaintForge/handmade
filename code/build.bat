@echo off

mkdir ..\build
pushd ..\build
call "D:\Visual Studio\VC\vcvarsall.bat" x64
cl -Zi w:\handmade\code\win32_handmade.cpp user32.lib Gdi32.lib
popd

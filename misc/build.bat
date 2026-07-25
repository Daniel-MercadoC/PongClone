@echo off

pushd ..\build

cl -o ..\pong ..\src\main.c ..\src\game.c ..\src\ui.c -I ..\include\ /link /subsystem:windows /entry:mainCRTStartup ..\lib\raylib.lib gdi32.lib User32.lib shell32.lib winmm.lib opengl32.lib kernel32.lib msvcrt.lib ..\res\pong.res

popd

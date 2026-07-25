@echo off

subst w: "path/to/filesystem"

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

set path=w:\misc\;%path%

w:

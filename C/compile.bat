@echo off

clang.exe -I include -I include\SDL -o png-parser.exe src\main.c -L dll -l sdl2 -l zlibwapi
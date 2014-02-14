@echo off
luajit -b loader.lua loader.lc
luajit bin2c.lua loader.lc >loader.c
gcc -w -c lua2exe.c -o obj\lua2exe.o -s -O3 -Os
gcc obj\lua2exe.o -o lua2exe.exe -lluajit -lzlib -s -O3 -Os
gcc -w -c lua.c -o obj\lua.o -s -O3 -Os
gcc obj\lua.o -o lua.lvm -lluajit -lzlib -s -O3 -Os
upx -9 -q lua.lvm |null
gcc obj\lua.o -o luaw.lvm -lluajit -lzlib -mwindows -s -O3 -Os
upx -9 -q luaw.lvm |null
zip -9 -u -q lua2exe.zip lua2exe.exe *.dll *.lvm readme.txt
zip -9 -u -q lua2exe-src.zip lua2exe.exe *.dll *.lvm *.c *.h bin2c.lua loader.lc loader.lua readme.txt
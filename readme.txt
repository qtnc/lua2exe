lua2exe

# What is lua2exe ?
As his name suggests, lua2exe convert a serie of lua scripts into a standalone executable.
The generated executable still need lua shared library, lua51.dll on windows.

# How to use lua2exe ?
To make an executable out of your lua scripts, just run lua2exe and specifiy which scripts to embed. It's very easy.

For example :
lua2exe -o myprog.exe script1.lua script2.lua script3.lua
will generate myprog.exe out of script1.lua, script2.lua and script3.lua.

Only the first script given will be run when the generated executable starts. In our case script1.lua.
Other embedded scripts can be called by using require or dofile.
You can make a mixed executable that runs both embedded and external scripts.
If a script with the same file name as the one embedded into the executable really exists in the current directory, the embedded script has always the priority. This is safer.

You can decide to keep debug information in lua scripts by adding the -g option.
You can use different base VM binaries by using the -b option . Two versions of the lua VM are provided :
* lua.lvm: default console VM
* luaw.lvm: VM to run GUI applications (without console)
Run lua2exe -h to see more command-line options.

# Where it works ?
Currently, lua2exe works only on windows. You are welcome to propose a patch to make it working on linux, mac, or whatever else.

# How it works ?
The scripts given to lua2exe are compiled and compressed, then finally appended to the end of a base executable image to form a single executable embedding the scripts.
The base image is quite similar to the lua interpreter, except that it doesn't includes the interactive mode.
At startup, the list of embedded files is fetched from the executable itself, and the first embedded script is loaded and run.

Important note: lua code in generated executables is compiled and compressed, not emcrypted ! It means that you have absolutely no protection against decompilation and reverse engineering of produced executables.

# Copyright
Lua2exe is copyright © 2012, QuentinC (http://quentinc.net/)

# Credits
* Luajit 2.0.0.10 as lua VM by Mike Pal http://luajit.org/
* Zlib for bytecode compression by Jean-loup Gailly and Marc Adler http://zlib.net/

# License
Lua2exe itself is licensed under GPL.
Executables produced by lua2exe don't necessarily have to be in GPL.

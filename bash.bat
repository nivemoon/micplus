windres res/micplus.rc -O coff -o micplus.res
gcc src/micplus.c micplus.res -o bin/micplus.exe -mwindows -lole32 -luuid -luser32 -lshell32

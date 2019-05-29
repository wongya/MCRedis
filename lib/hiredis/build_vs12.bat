@echo on

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\bin\x86_amd64\vcvarsx86_amd64.bat"

cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" async.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" dict.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" hiredis.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" net.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" read.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" sds.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" /FI"errno.h" sockcompat.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" /FI"stdarg.h" /FI"win32.h" sslio.c

lib async.obj dict.obj hiredis.obj net.obj read.obj sds.obj sockcompat.obj sslio.obj /OUT:hiredis_vs12.lib /MACHINE:X64

@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvars64.bat"

cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" async.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" dict.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" hiredis.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" net.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" read.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" sds.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" sockcompat.c
cl /D "WIN32" /c /MTd /Zi /Fd"hiredis.pdb" sslio.c

lib async.obj dict.obj hiredis.obj net.obj read.obj sds.obj sockcompat.obj sslio.obj /OUT:hiredis_vs17.lib /MACHINE:X64
MIPS_DRIVE=e:

INCPATH="$(MIPS_DRIVE)\Program Files\DevStudio\WCE\include\WCE200"
LIBPATH="$(MIPS_DRIVE)\Program Files\DevStudio\wce\lib\wce200\wcemips"
CC=clmips -c /I$(INCPATH) /Ox /GF /nologo /Fatest.s
LINK=link /nologo /machine:MIPS /SUBSYSTEM:WINDOWSCE /entry:main \
	/libpath:$(LIBPATH) /nodefaultlib /base:0x00000000 

all: convert.exe mips.exe test.exe

convert.exe: convert.c
	cl convert.c

mips.exe: mips.c
	cl mips.c

test.exe: test.c
	$(CC) test.c -D SIMULATE
	$(LINK) /out:test.exe /map:test.map test.obj
	convert
	mips test.exe DD > test.txt
	mips test2.exe DD > test2.txt
	mips test2.exe



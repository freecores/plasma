# NOTE:  CC is for the x86 compiler to compile the PC based tools.
# Gcc is for the MIPS test code.
# If you get an error using GNU make.exe try a different make utility.
# The MIPS gcc compiler libraries cause problems with GNU make.

CC = cl /O1
#CC = gcc_x86 -O

all: convert.exe mlite.exe tracehex.exe bintohex.exe
	@echo make targets = count, opcodes, pi, test, run, tohex

convert.exe: convert.c
	@$(CC) -o convert.exe convert.c

mlite.exe: mlite.c
	@$(CC) -o mlite.exe mlite.c

tracehex.exe: tracehex.c
	@$(CC) -o tracehex.exe tracehex.c

bintohex.exe: bintohex.c
	@$(CC) -o bintohex.exe bintohex.c

tohex: 
	bintohex test2.exe 
	copy code*.hex ..\vhdl

opcodes: all
	as -o opcodes.o opcodes.asm
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe opcodes.o
	-@objdump --disassemble test.exe > test.lst
	@convert
	-@mlite test2.exe BD > test.txt
	@copy code.txt ..\vhdl

test: all
	as -o boot.o boot.asm
	gcc -O -DMLITE -Dmain=main2 -Wall -c -s test.c
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe boot.o test.o
	-@objdump --disassemble test.exe > test.lst
	@convert
	-@mlite test2.exe BD > test.txt
	@copy code.txt ..\vhdl

count: all
	as -o boot.o boot.asm
	gcc -O -DMLITE -Dmain=main2 -Wall -c -s count.c
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe boot.o count.o
	-objdump --disassemble test.exe > test.lst
	convert
	-mlite test2.exe BD > test.txt
	copy code.txt ..\vhdl

pi: all
	as -o boot.o boot.asm
	gcc -O -DMLITE -Dmain=main2 -Wall -c -s pi.c
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe boot.o pi.o 
	@objdump --disassemble test.exe > test.lst
	@convert
	-@mlite test2.exe BD > test.txt
	@copy code.txt ..\vhdl

run: mlite.exe
	@mlite test2.exe B



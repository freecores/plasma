all: convert.exe mips.exe tracehex.exe 

convert.exe: convert.c
	@cl convert.c

mips.exe: mips.c
	@cl mips.c

tracehex.exe: tracehex.c
	@cl tracehex.c

opcodes: 
	as -o opcodes.o opcodes.asm
	ld -Ttext 0 -eentry -otest.exe opcodes.o
	@objdump --disassemble-all test.exe > test.lst
	@convert
	-@mips test2.exe BD > test.txt
	@copy code.txt ..\vhdl

test: 
	@as -o boot.o boot.asm
	gcc -DMIPS -c -s test.c
	ld -Ttext 0 -eentry -otest.exe boot.o test.o
	@objdump --disassemble-all test.exe > test.lst
	@convert
	-@mips test2.exe BD > test.txt
	@copy code.txt ..\vhdl

run: code.txt mips.exe
	@mips test2.exe B




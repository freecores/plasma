all: convert.exe mlite.exe tracehex.exe 

convert.exe: convert.c
	@cl convert.c

mlite.exe: mlite.c
	@cl mlite.c

tracehex.exe: tracehex.c
	@cl tracehex.c

opcodes: all
	as -o opcodes.o opcodes.asm
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe opcodes.o
	-@objdump --disassemble test.exe > test.lst
	@convert
	-@mlite test2.exe BD > test.txt
	@copy code.txt ..\vhdl

test: all
	as -o boot.o boot.asm
	gcc -DMIPS -Dmain=main2 -Wall -c -s test.c
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe boot.o test.o
	-@objdump --disassemble test.exe > test.lst
	@convert
	-@mlite test2.exe BD > test.txt
	@copy code.txt ..\vhdl

count: all
	as -o boot.o boot.asm
	gcc -DMIPS -Dmain=main2 -Wall -c -s count.c
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe boot.o count.o
	-@objdump --disassemble test.exe > test.lst
	@convert
	-@mlite test2.exe BD > test.txt
	@copy code.txt ..\vhdl

pi: all
	as -o boot.o boot.asm
	gcc -DMIPS -Dmain=main2 -Wall -c -s pi.c
	ld -Ttext 0 -eentry -Map test.map -s -N -o test.exe boot.o pi.o 
	@objdump --disassemble test.exe > test.lst
	@convert
	-@mlite test2.exe BD > test.txt
	@copy code.txt ..\vhdl

run: mlite.exe
	@mlite test2.exe B




# The MIPS gcc compiler must use the cygwin1.dll that came with the compiler.
# The CC_X86 is for compiling tools on your PC.
# The GCC_MIPS is for compiling code on the target.
# Convert_bin changes test.exe into code.txt which is used by the VHDL.

#CC_X86 = gcc_x86 -O
CC_X86 = cl /O1

CFLAGS = -O2 -Wall -c -s  
GCC_MIPS = gcc $(CFLAGS)
AS_MIPS = as
LD_MIPS = ld
DUMP_MIPS = objdump

all: convert_bin.exe mlite.exe tracehex.exe bintohex.exe ram_image.exe
	@echo make targets = count, opcodes, pi, test, run, tohex, toimage, rtos

clean:
	-del *.o *.obj *.exe *.map *.lst *.hex *.txt

convert_bin.exe: convert.c
	@$(CC_X86) -o convert_bin.exe convert.c

mlite.exe: mlite.c
	@$(CC_X86) -o mlite.exe mlite.c -DWIN32

tracehex.exe: tracehex.c
	@$(CC_X86) -o tracehex.exe tracehex.c

bintohex.exe: bintohex.c
	@$(CC_X86) -o bintohex.exe bintohex.c

ram_image.exe: ram_image.c
	@$(CC_X86) -o ram_image.exe ram_image.c

opcodes: all
	$(AS_MIPS) -o opcodes.o opcodes.asm
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe opcodes.o
	-@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

opcodes2: all
	$(AS_MIPS) -o opcodes.o opcodes.asm
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.exe opcodes.o
	-@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

test: all
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) test.c
	$(GCC_MIPS) no_os.c
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o test.o no_os.o
	-@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

count: all
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) count.c
	$(GCC_MIPS) no_os.c 
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o count.o no_os.o 
	-$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

count2: all
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) count.c
	$(GCC_MIPS) no_os.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.exe \
		boot.o count.o no_os.o
	-$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

pi: all
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) pi.c
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o pi.o 
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

pi2: all
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) pi.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.exe \
		boot.o pi.o 
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

bootldr: all
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) bootldr.c
	$(GCC_MIPS) no_os.c
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o bootldr.o no_os.o
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin
	copy code.txt ..\vhdl

# Run a MIPS opcode simulator
run: mlite.exe
	@mlite test.bin 

disassemble:
	-@mlite test.bin BD > test.txt

# Used by Altera FPGAs for ram image
tohex: 
	bintohex test2.exe 
	copy code*.hex ..\vhdl

# Used by Xilinx FPGAs for ram image
toimage:
	ram_image ..\vhdl\ram_xilinx.vhd ..\vhdl\code.txt ..\vhdl\ram_image.vhd



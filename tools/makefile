# The MIPS gcc compiler must use the cygwin1.dll that came with the compiler.
# The CC_X86 is for compiling tools on your PC.
# The GCC_MIPS is for compiling code on the target.
# Convert_bin changes test.exe into code.txt which is used by the VHDL.

# Customize for Linux 
#CC_X86 = gcc -Wall -O -g
#CP = cp
#RM = rm -rf 
#BIN_MIPS = ../../mips/sde/bin
#VHDL_DIR = ../vhdl

# Customize for Windows
CC_X86 = cl /O1
CP = copy
RM = del
BIN_MIPS = ../gccmips_elf
VHDL_DIR = ..\vhdl

CFLAGS = -O2 -Wall -c -s  

GCC_MIPS  = $(BIN_MIPS)/gcc $(CFLAGS)
AS_MIPS   = $(BIN_MIPS)/as
LD_MIPS   = $(BIN_MIPS)/ld
DUMP_MIPS = $(BIN_MIPS)/objdump

all: convert_bin.exe tracehex.exe bintohex.exe ram_image.exe
	@echo make targets = count, opcodes, pi, test, run, tohex, toimage, rtos

clean:
	-$(RM) *.o *.obj *.map *.lst *.hex *.txt

convert_bin.exe: convert.c
	@$(CC_X86) -o convert_bin.exe convert.c

convert_le.exe: convert.c
	@$(CC_X86) -DLITTLE_ENDIAN -o convert_le.exe convert.c

mlite.exe: mlite.c
	@$(CC_X86) -o mlite.exe mlite.c -DWIN32

tracehex.exe: tracehex.c
	@$(CC_X86) -o tracehex.exe tracehex.c

bintohex.exe: bintohex.c
	@$(CC_X86) -o bintohex.exe bintohex.c

ram_image.exe: ram_image.c
	@$(CC_X86) -o ram_image.exe ram_image.c

opcodes: 
	$(AS_MIPS) -o opcodes.o opcodes.asm
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe opcodes.o
	-@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

opcodes2: 
	$(AS_MIPS) -o opcodes.o opcodes.asm
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.exe opcodes.o
	-@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

test: 
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) test.c
	$(GCC_MIPS) no_os.c
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o test.o no_os.o
	-@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

count: 
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) count.c
	$(GCC_MIPS) no_os.c 
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o count.o no_os.o 
	-$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

count2:
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) count.c
	$(GCC_MIPS) no_os.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.exe \
		boot.o count.o no_os.o
	-$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

pi: 
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) pi.c
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o pi.o 
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

pi2:
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) pi.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.exe \
		boot.o pi.o 
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

bootldr:
	$(AS_MIPS) -o boot.o boot.asm
	$(GCC_MIPS) bootldr.c
	$(GCC_MIPS) no_os.c
	$(LD_MIPS) -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o bootldr.o no_os.o
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_bin.exe
	$(CP) code.txt $(VHDL_DIR)

bootldr_little_endian: convert_le.exe
	$(AS_MIPS) -EL -o boot.o boot.asm
	$(GCC_MIPS) -EL bootldr.c
	$(GCC_MIPS) -EL no_os.c
	$(LD_MIPS) -EL -Ttext 0 -eentry -Map test.map -s -N -o test.exe \
		boot.o bootldr.o no_os.o
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	convert_le.exe
	$(CP) code.txt $(VHDL_DIR)

# Run a MIPS opcode simulator
run: mlite.exe
	@mlite.exe test.bin 

run_little_endian: mlite.exe
	@mlite.exe test.bin L

disassemble:
	-@mlite.exe test.bin BD > test.txt

# Used by Altera FPGAs for ram image
tohex: 
	bintohex.exe test2.exe 
	$(CP) code*.hex $(VHDL_DIR)

# Used by Xilinx FPGAs for ram image
toimage:
	ram_image.exe ../vhdl/ram_xilinx.vhd ../vhdl/code.txt ../vhdl/ram_image.vhd



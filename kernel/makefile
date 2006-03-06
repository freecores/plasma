# The MIPS gcc compiler must use the cygwin1.dll that came with the compiler.
# The CC_X86 is for compiling tools on your PC.
# The GCC_MIPS is for compiling code on the target.
# Convert_bin changes test.exe into code.txt which is used by the VHDL.

#CC_X86 = gcc_x86 -O
CC_X86 = cl /O1

#CFLAGS = -DMLITE -Dmain=main2 -Wall -c -s  
#CFLAGS = -O2 -DMLITE -Dmain=main2 -Wall -c -s  
CFLAGS = -O2 -Wall -c -s -I..\tools -msoft-float 
GCC_MIPS = gcc $(CFLAGS)
AS_MIPS = as
LD_MIPS = ld
DUMP_MIPS = objdump
#GCC_MIPS = gcc_mips $(CFLAGS)
#AS_MIPS = as_mips
#LD_MIPS = ld_mips

clean:
	-del *.o *.obj *.exe *.map *.lst *.hex *.txt

rtos: 
	$(AS_MIPS) -o boot.o ..\tools\boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c
	$(GCC_MIPS) math.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.exe \
		boot.o rtos.o libc.o uart.o rtos_test.o math.o
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.exe > test.lst
	..\tools\convert_bin


# Run a MIPS opcode simulator
run: 
	@..\tools\mlite test.bin 

disassemble:
	-@..\tools\mlite test.bin BD > test.txt



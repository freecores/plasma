# The MIPS gcc compiler must use the cygwin1.dll that came with the compiler.

CFLAGS = -O2 -Wall -c -s -I..\tools -msoft-float 
GCC_MIPS = gcc $(CFLAGS)
AS_MIPS = as
LD_MIPS = ld
DUMP_MIPS = objdump

rtos: 
	$(AS_MIPS) -o boot.o ..\tools\boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c
	$(GCC_MIPS) math.c
	$(GCC_MIPS) debugger.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o   
	..\tools\convert_bin
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst
	cp test.bin ..\tools

clean:
	-del *.o *.obj *.exe *.map *.lst *.hex *.txt

# Run a MIPS opcode simulator
run: 
	@..\tools\mlite test.bin 

disassemble:
	-@..\tools\mlite test.bin BD > test.txt



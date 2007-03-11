# The MIPS gcc compiler must use the cygwin1.dll that came with the compiler.

# Use software multiplier (don't use mult.vhd)
CFLAGS_SW_MULT = -mno-mul -DUSE_SW_MULT

# Use 16 fewer registers (make reg_bank.vhd smaller)
CFLAGS_FEW_REGS = -ffixed-t1 -ffixed-t2 -ffixed-t3 -ffixed-t4 -ffixed-t5 -ffixed-t6 -ffixed-t7 -ffixed-t8 -ffixed-t9 -ffixed-s2 -ffixed-s3 -ffixed-s4 -ffixed-s5 -ffixed-s6 -ffixed-s7 -ffixed-s8 

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



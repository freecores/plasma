# The MIPS gcc compiler must use the cygwin1.dll that came with the compiler.

# Customize for Linux
#CC_X86 = gcc -Wall -O -g
#CP = cp
#RM = rm -rf
#BIN_MIPS = ../../mips/sde/bin
#VHDL_DIR = ../vhdl
#GCC_MIPS  = $(BIN_MIPS)/gcc $(CFLAGS)
#AS_MIPS   = $(BIN_MIPS)/as
#LD_MIPS   = $(BIN_MIPS)/ld
#DUMP_MIPS = $(BIN_MIPS)/objdump

# Customize for Windows
CC_X86 = cl /O1
CP = copy
RM = del
BIN_MIPS = ..\gccmips_elf
VHDL_DIR = ..\vhdl
GCC_MIPS  = $(BIN_MIPS)\gcc $(CFLAGS)
AS_MIPS   = $(BIN_MIPS)\as
LD_MIPS   = $(BIN_MIPS)\ld
DUMP_MIPS = $(BIN_MIPS)\objdump

# Use software multiplier (don't use mult.vhd)
CFLAGS_SW_MULT = -mno-mul -DUSE_SW_MULT

# Use 16 fewer registers (make reg_bank.vhd smaller)
CFLAGS_FEW_REGS = -ffixed-t1 -ffixed-t2 -ffixed-t3 -ffixed-t4 -ffixed-t5 -ffixed-t6 -ffixed-t7 -ffixed-t8 -ffixed-t9 -ffixed-s2 -ffixed-s3 -ffixed-s4 -ffixed-s5 -ffixed-s6 -ffixed-s7 -ffixed-s8 

CFLAGS = -O2 -Wall -c -s -I..\tools -msoft-float 

rtos: 
	$(AS_MIPS) -o boot.o ..\tools\boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c
	$(GCC_MIPS) math.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o 
	..\tools\convert_bin
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst
	cp test.bin ..\tools

rtosfull: 
	$(AS_MIPS) -o boot.o ..\tools\boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c -DINCLUDE_HTML -DINCLUDE_CONSOLE
	$(GCC_MIPS) math.c
	$(GCC_MIPS) tcpip.c
	$(GCC_MIPS) http.c
	$(GCC_MIPS) netutil.c
	$(GCC_MIPS) filesys.c
	$(GCC_MIPS) -I. ..\app\html.c -DMainThread=HtmlThread
	$(GCC_MIPS) -I. ..\app\image.c
	$(GCC_MIPS) -I. ..\app\tictac.c
	$(GCC_MIPS) -I. ..\app\tic3d.c
	$(GCC_MIPS) -I. ..\app\connect4.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o tcpip.o \
	http.o netutil.o filesys.o html.o image.o tictac.o tic3d.o connect4.o
	..\tools\convert_bin
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst
	cp test.bin ..\tools

clean:
	-$(RM) *.o *.obj *.axf *.map *.lst *.hex *.txt *.bin

# Run a MIPS opcode simulator
run: 
	@..\tools\mlite test.bin 

disassemble:
	-@..\tools\mlite test.bin BD > test.txt



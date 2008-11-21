# Makefile for Plasma RTOS and Plasma TCP/IP stack

ifeq ($(LANG),)

# Customize for Windows
# The MIPS gcc compiler must use the cygwin1.dll that came with the compiler.
CC_X86 = cl /O1 /nologo /I..\tools
CP = copy
RM = del
DWIN32 = -DWIN32
BIN_MIPS = ..\gccmips_elf
TOOLS_DIR = ..\tools\\
APP_DIR = ..\App\\
LINUX_PWD =
ALIASING  =
GCC_MIPS  = $(BIN_MIPS)\gcc $(CFLAGS)
AS_MIPS   = $(BIN_MIPS)\as
LD_MIPS   = $(BIN_MIPS)\ld
DUMP_MIPS = $(BIN_MIPS)\objdump
CONVERT_BIN = $(TOOLS_DIR)convert_bin.exe
OBJ = obj
CFLAGS_X86 = /c /DWIN32
LFLAGS_X86 =

else

# Customized for Linux
# See http://www.opencores.com/projects.cgi/web/mips/linux_tools.htm
CC_X86 = gcc -Wall -O -g -I../tools
CP = cp
RM = rm -rf 
DWIN32 =
TOOLS_DIR = ../tools/
APP_DIR = ../App/
LINUX_PWD = ./
ALIASING  = -fno-strict-aliasing
GCC_MIPS  = $(BIN_MIPS)mips-elf-gcc $(CFLAGS)
AS_MIPS   = $(BIN_MIPS)mips-elf-as
LD_MIPS   = $(BIN_MIPS)mips-elf-ld
DUMP_MIPS = $(BIN_MIPS)mips-elf-objdump
CONVERT_BIN = $(TOOLS_DIR)convert_bin.exe
#CONVERT_BIN = $(BIN_MIPS)mips-elf-objcopy -I elf32-big -O binary test.axf test.bin
OBJ = o
CFLAGS_X86 = -c -DWIN32 -DLINUX
LFLAGS_X86 = -lm

endif

# Use software multiplier (don't use mult.vhd)
CFLAGS_SW_MULT = -mno-mul -DUSE_SW_MULT

# Use 16 fewer registers (make reg_bank.vhd smaller)
CFLAGS_FEW_REGS = -ffixed-t0 -ffixed-t1 -ffixed-t2 -ffixed-t3 -ffixed-t4 -ffixed-t5 -ffixed-t6 -ffixed-t7 -ffixed-s0 -ffixed-s1 -ffixed-s2 -ffixed-s3 -ffixed-s4 -ffixed-s5 -ffixed-s6 -ffixed-s7 

CFLAGS = -O2 -Wall -c -s -I$(TOOLS_DIR) -msoft-float -fno-builtin 
#CFLAGS += $(CFLAGS_SW_MULT)
#CFLAGS += $(CFLAGS_FEW_REGS)

# Build just the Plasma RTOS for Plasma CPU
rtos: 
	$(AS_MIPS) -o boot.o $(TOOLS_DIR)boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c
	$(GCC_MIPS) math.c $(ALIASING)
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o 
	$(CONVERT_BIN)
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst

rtos_nomult: 
	$(AS_MIPS) -o boot.o $(TOOLS_DIR)boot.asm
	$(AS_MIPS) -o mult_sim.o ..\tools\mult_sim.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c
	$(GCC_MIPS) math.c $(ALIASING)
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o mult_sim.o
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst
	..\tools\nomult test.axf test.lst test.map
	@$(DUMP_MIPS) --disassemble test.axf > test2.lst
	$(CONVERT_BIN)

# Build the Plasma RTOS, Plasma TCP/IP stack, and web server for the Plasma CPU
rtos_tcpip:
	$(AS_MIPS) -o boot.o $(TOOLS_DIR)boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c -DINCLUDE_HTML -DINCLUDE_CONSOLE
	$(GCC_MIPS) math.c $(ALIASING)
	$(GCC_MIPS) tcpip.c
	$(GCC_MIPS) http.c -DINCLUDE_FILESYS -DEXAMPLE_HTML
	$(GCC_MIPS) netutil.c
	$(GCC_MIPS) filesys.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o tcpip.o \
	http.o netutil.o filesys.o
	$(CONVERT_BIN)
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst

rtos_tcpip_eth:
	$(AS_MIPS) -o boot.o $(TOOLS_DIR)boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c -DNO_PACKETS
	$(GCC_MIPS) rtos_test.c -DINCLUDE_ETH -DINCLUDE_CONSOLE
	$(GCC_MIPS) math.c $(ALIASING)
	$(GCC_MIPS) tcpip.c
	$(GCC_MIPS) http.c -DINCLUDE_FILESYS -DEXAMPLE_HTML
	$(GCC_MIPS) netutil.c
	$(GCC_MIPS) filesys.c
	$(GCC_MIPS) ethernet.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o tcpip.o \
	http.o netutil.o filesys.o ethernet.o
	$(CONVERT_BIN)
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst

# Build full test application
rtosuart: 
	$(AS_MIPS) -o boot.o $(TOOLS_DIR)boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c
	$(GCC_MIPS) rtos_test.c -DINCLUDE_HTML -DINCLUDE_CONSOLE
	$(GCC_MIPS) math.c $(ALIASING)
	$(GCC_MIPS) tcpip.c
	$(GCC_MIPS) http.c -DINCLUDE_FILESYS
	$(GCC_MIPS) netutil.c
	$(GCC_MIPS) filesys.c
	$(GCC_MIPS) -I. $(APP_DIR)html.c -DMainThread=HtmlThread
	$(GCC_MIPS) -I. $(APP_DIR)image.c
	$(GCC_MIPS) -I. $(APP_DIR)tictac.c
	$(GCC_MIPS) -I. $(APP_DIR)tic3d.c
	$(GCC_MIPS) -I. $(APP_DIR)connect4.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o tcpip.o \
	http.o netutil.o filesys.o html.o image.o tictac.o tic3d.o connect4.o 
	$(CONVERT_BIN)
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst

# Build full test application
rtosfull: 
	$(AS_MIPS) -o boot.o $(TOOLS_DIR)boot.asm
	$(GCC_MIPS) rtos.c
	$(GCC_MIPS) libc.c
	$(GCC_MIPS) uart.c -DNO_PACKETS
	$(GCC_MIPS) rtos_test.c -DINCLUDE_ETH -DINCLUDE_CONSOLE
	$(GCC_MIPS) math.c $(ALIASING)
	$(GCC_MIPS) tcpip.c
	$(GCC_MIPS) http.c -DINCLUDE_FILESYS
	$(GCC_MIPS) netutil.c -DINCLUDE_FLASH -DDLL_SETUP
	$(GCC_MIPS) filesys.c -DINCLUDE_FLASH
	$(GCC_MIPS) ethernet.c
	$(GCC_MIPS) flash.c
	$(GCC_MIPS) -I. $(APP_DIR)html.c -DMainThread=HtmlThread
	$(GCC_MIPS) -I. $(APP_DIR)image.c
	$(GCC_MIPS) -I. $(APP_DIR)tictac.c
	$(GCC_MIPS) -I. $(APP_DIR)tic3d.c
	$(GCC_MIPS) -I. $(APP_DIR)connect4.c
	$(LD_MIPS) -Ttext 0x10000000 -eentry -Map test.map -s -N -o test.axf \
	boot.o rtos.o libc.o uart.o rtos_test.o math.o tcpip.o \
	http.o netutil.o filesys.o ethernet.o flash.o \
	html.o image.o tictac.o tic3d.o connect4.o 
	$(CONVERT_BIN)
	@sort <test.map >test2.map
	@$(DUMP_MIPS) --disassemble test.axf > test.lst

# Test the RTOS running on a PC
testrtos:
	@$(CC_X86) $(CFLAGS_X86) rtos.c
	@$(CC_X86) $(CFLAGS_X86) libc.c -fno-builtin
	@$(CC_X86) $(CFLAGS_X86) uart.c
	@$(CC_X86) $(CFLAGS_X86) rtos_test.c
	@$(CC_X86) $(CFLAGS_X86) math.c $(ALIASING)
	@$(CC_X86) $(LFLAGS_X86) -o testrtos.exe rtos.$(OBJ) libc.$(OBJ) uart.$(OBJ) rtos_test.$(OBJ) math.$(OBJ) 
	$(LINUX_PWD)testrtos.exe

# Test the TCP/IP protocol stack running on a PC
testip:
	@$(CC_X86) $(CFLAGS_X86) tcpip.c
	@$(CC_X86) $(CFLAGS_X86) http.c /DEXAMPLE_HTML
	@$(CC_X86) $(CFLAGS_X86) netutil.c
	@$(CC_X86) $(CFLAGS_X86) filesys.c
	@$(CC_X86) $(CFLAGS_X86) libc.c /I$(TOOLS_DIR) -fno-builtin
	@$(CC_X86) $(CFLAGS_X86) /DSIMULATE_PLASMA $(TOOLS_DIR)etermip.c
	@$(CC_X86) $(CFLAGS_X86)  os_stubs.c
	@$(CC_X86) -o testip.exe etermip.obj $(TOOLS_DIR)wpcap.lib \
	tcpip.obj http.obj netutil.obj filesys.obj libc.c os_stubs.c
	@echo Try http://plasmb/.  Try telnet plasmb.  Try ftp plasmb.
	$(LINUX_PWD)testip.exe

clean:
	-$(RM) *.o *.obj *.axf *.map *.lst *.hex *.txt *.bin *.exe

# Run a Plasma CPU opcode simulator (can execute rtos target)
run: 
	@$(TOOLS_DIR)mlite.exe test.bin 

disassemble:
	-@$(TOOLS_DIR)mlite.exe test.bin BD > test.txt

# Start the EtermIP terminal program to download the code to the Plasma CPU
# and permit an Ethernet packets to be transfered.
download:
	@echo Reset board before downloading code
	$(TOOLS_DIR)etermip.exe



##################################################################
# TITLE: Boot Up Code
# AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
# DATE CREATED: 1/12/02
# FILENAME: boot.asm
# PROJECT: MIPS CPU core
# COPYRIGHT: Software placed into the public domain by the author.
#    Software 'as is' without warranty.  Author liable for nothing.
# DESCRIPTION:
#    Initializes the stack pointer and jumps to main2().
##################################################################
	.text
	.align	2
	.globl	entry
	.ent	entry
entry:
   .set noreorder

   ori   $sp,$0,0x8000     #initialize stack pointer
   ori   $4,$0,1
   mtc0  $4,$12            #STATUS=1; enable interrupts
	jal	main2
   nop
$L1:
   j $L1
   nop
   nop

isr_storage:        #address 0x20
   nop
   nop
   nop
   nop

   #address 0x30
interrupt_service_routine:
   sw $4,-4($sp)

   sw $5,-8($sp)
   ori $5,$0,0xffff
   ori $4,$0,46
   sb $4,0($5)      #echo out '.'
   lw $5,-8($sp)
   
   #normally clear the interrupt source here
   #re-enable interrupts
   ori $4,$0,0x1
   mtc0 $4,$12      #STATUS=1; enable interrupts

   #FIXME there is a small race condition here!

   mfc0 $4,$14      #C0_EPC=14
   j $4
   lw $4,-4($sp)

   .set reorder
	.end	entry


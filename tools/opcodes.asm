##################################################################
# TITLE: Opcode Tester
# AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
# DATE CREATED: 1/10/02
# FILENAME: opcodes.asm
# PROJECT: MIPS CPU core
# COPYRIGHT: Software placed into the public domain by the author.
#    Software 'as is' without warranty.  Author liable for nothing.
# DESCRIPTION:
#    This assembly file tests all of the opcodes supported by the
#    MIPS-lite core.
#    This test assumes that address 0xffff is the UART write register
#    Successful tests will print out "A" or "AB" or "ABC" or ....
#    Missing letters or letters out of order indicate a failure.
##################################################################
	.text
	.align	2
	.globl	entry
	.ent	entry
entry:
   .set noreorder

#   ori   $20,$0,0xffff      #serial port write address
#   li    $2,-66*19
#   ori   $3,$0,19
#   div   $2,$3
#   nop
#   mflo  $4
#   sub   $4,$0,$4
#   sb    $4,0($20)


   ori   $20,$0,0xffff      #serial port write address
   ori   $21,$0,'\n'        #<CR> letter
   ori   $22,$0,'X'         #'X' letter

   ######################################
   #Arithmetic Instructions
   ######################################
   ori   $2,$0,'A'
   sb    $2,0($20)
   ori   $2,$0,'r'
   sb    $2,0($20)
   ori   $2,$0,'i'
   sb    $2,0($20)
   ori   $2,$0,'t'
   sb    $2,0($20)
   ori   $2,$0,'h'
   sb    $2,0($20)
   sb    $21,0($20)

   #ADD
   ori   $3,$0,5
   ori   $4,$0,60
   add   $2,$3,$4
   sb    $2,0($20)
   sb    $21,0($20)

   #ADDI
   ori   $4,$0,60
   addi  $2,$4,5
   sb    $2,0($20)
   sb    $21,0($20)

   #ADDIU
   ori   $4,$0,50
   addiu $5,$4,15
   sb    $5,0($20)
   sb    $21,0($20)

   #ADDU
   ori   $3,$0,5
   ori   $4,$0,60
   add   $2,$3,$4
   sb    $2,0($20)
   sb    $21,0($20)

   #DIV
   ori   $2,$0,65*13
   ori   $3,$0,13
   div   $2,$3
   nop
   mflo  $4
   sb    $4,0($20)
   li    $2,-66*19
   ori   $3,$0,19
   div   $2,$3
   nop
   mflo  $4
   sub   $4,$0,$4
   sb    $4,0($20)
   ori   $2,$0,67*23
   li    $3,-23
   div   $2,$3
   nop
   mflo  $4
   sub   $4,$0,$4
   sb    $4,0($20)
   li    $2,-68*13
   li    $3,-13
   div   $2,$3
   mflo  $4
   sb    $4,0($20)
   sb    $21,0($20)

   #DIVU
   ori   $2,$0,65*13
   ori   $3,$0,13
   div   $2,$3
   nop
   mflo  $4
   sb    $4,0($20)
   sb    $21,0($20)

   #MULT
   ori   $2,$0,5
   ori   $3,$0,13
   mult  $2,$3
   nop
   mflo  $4
   sb    $4,0($20)
   li    $2,-5
   ori   $3,$0,13
   mult  $2,$3
   nop
   mflo  $4
   sub   $4,$0,$4
   addi  $4,$4,1
   sb    $4,0($20)
   ori   $2,$0,5
   li    $3,-13
   mult  $2,$3
   nop
   mflo  $4
   sub   $4,$0,$4
   addi  $4,$4,2
   sb    $4,0($20)
   li    $2,-5
   li    $3,-13
   mult  $2,$3
   nop
   mflo  $4
   addi  $4,$4,3
   sb    $4,0($20)
   sb    $21,0($20)

   #MULTU
   ori   $2,$0,5
   ori   $3,$0,13
   mult  $2,$3
   nop
   mflo  $4
   sb    $4,0($20)
   sb    $21,0($20)

   #SLT
   ori   $2,$0,10
   ori   $3,$0,12
   slt   $4,$2,$3
   addi  $5,$4,64
   sb    $5,0($20)
   slt   $4,$3,$2
   addi  $5,$4,66
   sb    $5,0($20)
   li    $2,0xfffffff0
   slt   $4,$2,$3
   addi  $5,$4,66
   sb    $5,0($20)
   slt   $4,$3,$2
   addi  $5,$4,68
   sb    $5,0($20)
   li    $3,0xffffffff
   slt   $4,$2,$3
   addi  $5,$4,68
   sb    $5,0($20)
   slt   $4,$3,$2
   addi  $5,$4,70
   sb    $5,0($20)
   sb    $21,0($20)

   #SLTI
   ori   $2,$0,10
   slti  $4,$2,12
   addi  $5,$4,64
   sb    $5,0($20)
   slti  $4,$2,8
   addi  $5,$4,66
   sb    $5,0($20)
   sb    $21,0($20)

   #SLTIU
   ori   $2,$0,10
   sltiu $4,$2,12
   addi  $5,$4,64
   sb    $5,0($20)
   sltiu $4,$2,8
   addi  $5,$4,66
   sb    $5,0($20)
   sb    $21,0($20)

   #SLTU
   ori   $2,$0,10
   ori   $3,$0,12
   slt   $4,$2,$3
   addi  $5,$4,64
   sb    $5,0($20)
   slt   $4,$3,$2
   addi  $5,$4,66
   sb    $5,0($20)
   sb    $21,0($20)

   #SUB
   ori   $3,$0,70
   ori   $4,$0,5
   sub   $2,$3,$4
   sb    $2,0($20)
   sb    $21,0($20)

   #SUBU
   ori   $3,$0,70
   ori   $4,$0,5
   sub   $2,$3,$4
   sb    $2,0($20)
   sb    $21,0($20)

   ######################################
   #Branch and Jump Instructions
   ######################################
   ori   $2,$0,'B'
   sb    $2,0($20)
   ori   $2,$0,'r'
   sb    $2,0($20)
   ori   $2,$0,'a'
   sb    $2,0($20)
   ori   $2,$0,'n'
   sb    $2,0($20)
   ori   $2,$0,'c'
   sb    $2,0($20)
   ori   $2,$0,'h'
   sb    $2,0($20)
   sb    $21,0($20)

   #B
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   b     $B1
   sb    $10,0($20)
   sb    $22,0($20)
$B1:
   sb    $11,0($20)
   sb    $21,0($20)

   #BAL
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $14,$0,'E'
   ori   $15,$0,'X'
   bal   $BAL1
   sb    $10,0($20)
   sb    $13,0($20)
   b     $BAL2
   sb    $14,0($20)
   sb    $15,0($20)
$BAL1:
   sb    $11,0($20)
   jr    $31
   sb    $12,0($20)
   sb    $22,0($20)
$BAL2:
   sb    $21,0($20)

   #BEQ
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $2,$0,100
   ori   $3,$0,123
   ori   $4,$0,123
   beq   $2,$3,$BEQ1
   sb    $10,0($20)
   sb    $11,0($20)
   beq   $3,$4,$BEQ1
   sb    $12,0($20)
   sb    $22,0($20)
$BEQ1:
   sb    $13,0($20)
   sb    $21,0($20)

   #BGEZ
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   or    $15,$0,'X'
   ori   $2,$0,100
   li    $3,0xffff1234
   ori   $4,$0,123
   bgez  $3,$BGEZ1
   sb    $10,0($20)
   sb    $11,0($20)
   bgez  $2,$BGEZ1
   sb    $12,0($20)
   sb    $22,0($20)
$BGEZ1:
   bgez  $0,$BGEZ2
   nop
   sb    $15,0($20)
$BGEZ2:
   sb    $13,0($20)
   sb    $21,0($20)

   #BGEZAL
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $14,$0,'E'
   ori   $15,$0,'X'
   li    $3,0xffff1234
   bgezal $3,$BGEZAL1
   nop
   sb    $10,0($20)
   bgezal $0,$BGEZAL1
   nop
   sb    $13,0($20)
   b     $BGEZAL2
   sb    $14,0($20)
   sb    $15,0($20)
$BGEZAL1:
   sb    $11,0($20)
   jr    $31
   sb    $12,0($20)
   sb    $22,0($20)
$BGEZAL2:
   sb    $21,0($20)

   #BGTZ
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $2,$0,100
   li    $3,0xffff1234
   bgtz  $3,$BGTZ1
   sb    $10,0($20)
   sb    $11,0($20)
   bgtz  $2,$BGTZ1
   sb    $12,0($20)
   sb    $22,0($20)
$BGTZ1:
   sb    $13,0($20)
   sb    $21,0($20)

   #BLEZ
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $2,$0,100
   li    $3,0xffff1234
   blez  $2,$BLEZ1
   sb    $10,0($20)
   sb    $11,0($20)
   blez  $3,$BLEZ1
   sb    $12,0($20)
   sb    $22,0($20)
$BLEZ1:
   blez  $0,$BLEZ2
   nop
   sb    $22,0($20)
$BLEZ2:
   sb    $13,0($20)
   sb    $21,0($20)

   #BLTZ
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $14,$0,'E'
   ori   $2,$0,100
   li    $3,0xffff1234
   ori   $4,$0,0
   bltz  $2,$BLTZ1
   sb    $10,0($20)
   sb    $11,0($20)
   bltz  $3,$BLTZ1
   sb    $12,0($20)
   sb    $22,0($20)
$BLTZ1:
   bltz  $4,$BLTZ2
   nop
   sb    $13,0($20)
$BLTZ2:
   sb    $14,0($20)
   sb    $21,0($20)

   #BLTZAL
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $14,$0,'E'
   ori   $15,$0,'X'
   li    $3,0xffff1234
   bltzal $0,$BLTZAL1
   nop
   sb    $10,0($20)
   bltzal $3,$BLTZAL1
   nop
   sb    $13,0($20)
   b     $BLTZAL2
   sb    $14,0($20)
   sb    $15,0($20)
$BLTZAL1:
   sb    $11,0($20)
   jr    $31
   sb    $12,0($20)
   sb    $22,0($20)
$BLTZAL2:
   sb    $21,0($20)

   #BNE
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $2,$0,100
   ori   $3,$0,123
   ori   $4,$0,123
   bne   $3,$4,$BNE1
   sb    $10,0($20)
   sb    $11,0($20)
   bne   $2,$3,$BNE1
   sb    $12,0($20)
   sb    $22,0($20)
$BNE1:
   sb    $13,0($20)
   sb    $21,0($20)

   #J
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $15,$0,'X'
   j     $J1
   sb    $10,0($20)
   sb    $15,0($20)
$J1:
   sb    $11,0($20)
   sb    $21,0($20)

   #JAL
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $14,$0,'E'
   ori   $15,$0,'X'
   jal   $JAL1
   sb    $10,0($20)
   sb    $13,0($20)
   b     $JAL2
   sb    $14,0($20)
   sb    $15,0($20)
$JAL1:
   sb    $11,0($20)
   jr    $31
   sb    $12,0($20)
   sb    $22,0($20)
$JAL2:
   sb    $21,0($20)

   #JALR
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $12,$0,'C'
   ori   $13,$0,'D'
   ori   $14,$0,'E'
   ori   $15,$0,'X'
   la    $3,$JALR1
   jalr  $3
   sb    $10,0($20)
   sb    $13,0($20)
   b     $JALR2
   sb    $14,0($20)
   sb    $15,0($20)
$JALR1:
   sb    $11,0($20)
   jr    $31
   sb    $12,0($20)
   sb    $22,0($20)
$JALR2:
   sb    $21,0($20)

   #JR
   ori   $10,$0,'A'
   ori   $11,$0,'B'
   ori   $15,$0,'X'
   la    $3,$JR1
   jr    $3
   sb    $10,0($20)
   sb    $15,0($20)
$JR1:
   sb    $11,0($20)
   sb    $21,0($20)

   #NOP
   ori   $2,$0,65
   nop
   sb    $2,0($20)
   sb    $21,0($20)

 
   ######################################
   #Load, Store, and Memory Control Instructions
   ######################################
   ori   $2,$0,'L'
   sb    $2,0($20)
   ori   $2,$0,'o'
   sb    $2,0($20)
   ori   $2,$0,'a'
   sb    $2,0($20)
   ori   $2,$0,'d'
   sb    $2,0($20)
   sb    $21,0($20)

   #LB
   ori   $2,$0,0xf000
   li    $3,0x41424344
   sw    $3,16($2)
   lb    $4,16($2)
   sb    $4,0($20)
   lb    $4,17($2)
   sb    $4,0($20)
   lb    $4,18($2)
   sb    $4,0($20)
   lb    $2,19($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #LBU
   ori   $2,$0,0xf000
   li    $3,0x41424344
   sw    $3,16($2)
   lb    $4,16($2)
   sb    $4,0($20)
   lb    $4,17($2)
   sb    $4,0($20)
   lb    $4,18($2)
   sb    $4,0($20)
   lb    $2,19($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #LH
   ori   $2,$0,0xf000
   li    $3,0x00410042
   sw    $3,16($2)
   lh    $4,16($2)
   sb    $4,0($20)
   lh    $2,18($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #LHU
   ori   $2,$0,0xf000
   li    $3,0x00410042
   sw    $3,16($2)
   lh    $4,16($2)
   sb    $4,0($20)
   lh    $2,18($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #LW
   ori   $2,$0,0xf000
   li    $3,'A'
   sw    $3,16($2)
   ori   $3,$0,0
   lw    $2,16($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #LWL & LWR
   ori   $2,$0,0xf000
   li    $3,'A'
   sw    $3,16($2)
   ori   $3,$0,0
   lwl   $2,16($2)
   lwr   $2,16($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #SB
   ori   $2,$0,'A'
   sb    $2,0($20)
   sb    $21,0($20)

   #SH
   ori   $2,$0,0xf000
   ori   $2,$0,0x4142
   sw    $2,16($2)
   lb    $3,16($2)
   sb    $3,0($20)
   lb    $2,17($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #SW
   ori   $2,$0,0xf000
   li    $3,0x41424344
   sw    $3,16($2)
   lb    $4,16($2)
   sb    $4,0($20)
   lb    $4,17($2)
   sb    $4,0($20)
   lb    $4,18($2)
   sb    $4,0($20)
   lb    $2,19($2)
   sb    $2,0($20)
   sb    $21,0($20)

   #SWL & SWR
   ori   $2,$0,0xf000
   li    $3,0x41424344
   swl   $3,16($2)
   swr   $3,16($2)
   lb    $4,16($2)
   sb    $4,0($20)
   lb    $4,17($2)
   sb    $4,0($20)
   lb    $4,18($2)
   sb    $4,0($20)
   lb    $2,19($2)
   sb    $2,0($20)
   sb    $21,0($20)


   ######################################
   #Logical Instructions
   ######################################
   ori   $2,$0,'L'
   sb    $2,0($20)
   ori   $2,$0,'o'
   sb    $2,0($20)
   ori   $2,$0,'g'
   sb    $2,0($20)
   ori   $2,$0,'i'
   sb    $2,0($20)
   ori   $2,$0,'c'
   sb    $2,0($20)
   sb    $21,0($20)

   #AND
   ori   $2,$0,0x0741
   ori   $3,$0,0x60f3
   and   $4,$2,$3
   sb    $4,0($20)
   sb    $21,0($20)

   #ANDI
   ori   $2,$0,0x0741
   andi  $4,$2,0x60f3
   sb    $4,0($20)
   sb    $21,0($20)

   #LUI
   lui   $2,0x41
   srl   $3,$2,16
   sb    $3,0($20)
   sb    $21,0($20)

   #NOR
   li    $2,0xf0fff08e
   li    $3,0x0f0f0f30
   nor   $4,$2,$3
   sb    $4,0($20)
   sb    $21,0($20)

   #OR
   ori   $2,$0,0x40
   ori   $3,$0,0x01
   or    $4,$2,$3
   sb    $4,0($20)
   sb    $21,0($20)

   #ORI
   ori   $2,$0,0x40
   ori   $4,$2,0x01
   sb    $4,0($20)
   sb    $21,0($20)

   #XOR
   ori   $2,$0,0xf043
   ori   $3,$0,0xf002
   xor   $4,$2,$3
   sb    $4,0($20)
   sb    $21,0($20)

   #XORI
   ori   $2,$0,0xf043
   xor   $4,$2,0xf002
   sb    $4,0($20)
   sb    $21,0($20)

 
   ######################################
   #Move Instructions
   ######################################
   ori   $2,$0,'M'
   sb    $2,0($20)
   ori   $2,$0,'o'
   sb    $2,0($20)
   ori   $2,$0,'v'
   sb    $2,0($20)
   ori   $2,$0,'e'
   sb    $2,0($20)
   sb    $21,0($20)

   #MFHI
   ori   $2,$0,65
   mthi  $2
   mfhi  $3
   sb    $3,0($20)
   sb    $21,0($20)

   #MFLO
   ori   $2,$0,65
   mtlo  $2
   mflo  $3
   sb    $3,0($20)
   sb    $21,0($20)

   #MTHI
   ori   $2,$0,65
   mthi  $2
   mfhi  $3
   sb    $3,0($20)
   sb    $21,0($20)

   #MTLO
   ori   $2,$0,65
   mtlo  $2
   mflo  $3
   sb    $3,0($20)
   sb    $21,0($20)


   ######################################
   #Shift Instructions
   ######################################
   ori   $2,$0,'S'
   sb    $2,0($20)
   ori   $2,$0,'h'
   sb    $2,0($20)
   ori   $2,$0,'i'
   sb    $2,0($20)
   ori   $2,$0,'f'
   sb    $2,0($20)
   ori   $2,$0,'t'
   sb    $2,0($20)
   sb    $21,0($20)

   #SLL
   li    $2,0x40414243
   sll   $3,$2,8
   srl   $3,$3,24
   sb    $3,0($20)
   sb    $21,0($20)

   #SLLV
   li    $2,0x40414243
   ori   $3,$0,8
   sllv  $3,$2,$3
   srl   $3,$3,24
   sb    $3,0($20)
   sb    $21,0($20)

   #SRA
   li    $2,0x40414243
   sra   $3,$2,16
   sb    $3,0($20)
   li    $2,0x84000000
   sra   $3,$2,25
   sub   $3,$3,0x80
   sb    $3,0($20)
   sb    $21,0($20)

   #SRAV
   li    $2,0x40414243
   ori   $3,$0,16
   srav  $3,$2,$3
   sb    $3,0($20)
   ori   $3,$0,25
   li    $2,0x84000000
   sra   $3,$2,$3
   sub   $3,$3,0x80
   sb    $3,0($20)
   sb    $21,0($20)

   #SRL
   li    $2,0x40414243
   srl   $3,$2,16
   sb    $3,0($20)
   li    $2,0x84000000
   sra   $3,$2,25
   sb    $3,0($20)
   sb    $21,0($20)

   #SRLV
   li    $2,0x40414243
   ori   $3,$0,16
   srlv  $4,$2,$3
   sb    $4,0($20)
   ori   $3,$0,25
   li    $2,0x84000000
   sra   $3,$2,$3
   sub   $3,$3,0x80
   sb    $3,0($20)
   sb    $21,0($20)


   ori   $2,$0,'D'
   sb    $2,0($20)
   ori   $2,$0,'o'
   sb    $2,0($20)
   ori   $2,$0,'n'
   sb    $2,0($20)
   ori   $2,$0,'e'
   sb    $2,0($20)
   sb    $21,0($20)


$DONE:
   j     $DONE
   nop

   .set reorder
	.end	entry


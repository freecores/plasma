---------------------------------------------------------------------
-- TITLE: Controller / Opcode Decoder
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 2/8/01
-- FILENAME: control.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- NOTE:  MIPS(tm) is a registered trademark of MIPS Technologies.
--    MIPS Technologies does not endorse and is not associated with
--    this project.
-- DESCRIPTION:
--    Controls the CPU by decoding the opcode and generating control 
--    signals to the rest of the CPU.
--    This entity decodes the MIPS(tm) opcode into a 
--    Very-Long-Word-Instruction.  
--    The 32-bit opcode is converted to a 
--       6+6+6+16+5+2+3+3+2+2+3+2+4 = 60 bit VLWI opcode.
--    Based on information found in:
--       "MIPS RISC Architecture" by Gerry Kane and Joe Heinrich
--       and "The Designer's Guide to VHDL" by Peter J. Ashenden
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use work.mlite_pack.all;

entity control is
   port(opcode       : in  std_logic_vector(31 downto 0);
        intr_signal  : in  std_logic;
        rs_index     : out std_logic_vector(5 downto 0);
        rt_index     : out std_logic_vector(5 downto 0);
        rd_index     : out std_logic_vector(5 downto 0);
        imm_out      : out std_logic_vector(15 downto 0);
        alu_func     : out alu_function_type;
        shift_func   : out shift_function_type;
        mult_func    : out mult_function_type;
        branch_func  : out branch_function_type;
        a_source_out : out a_source_type;
        b_source_out : out b_source_type;
        c_source_out : out c_source_type;
        pc_source_out: out pc_source_type;
        mem_source_out:out mem_source_type);
end; --entity control

architecture logic of control is
--   type alu_function_type is (alu_nothing, alu_add, alu_subtract, 
--      alu_less_than, alu_less_than_signed, 
--      alu_or, alu_and, alu_xor, alu_nor);
--   type shift_function_type is (
--      shift_nothing, shift_left_unsigned,  
--      shift_right_signed, shift_right_unsigned);
--   type mult_function_type is (
--      mult_nothing, mult_read_lo, mult_read_hi, mult_write_lo, 
--      mult_write_hi, mult_mult, mult_divide, mult_signed_divide);
--   type a_source_type is (from_reg_source, from_imm10_6);
--   type b_source_type is (from_reg_target, from_imm, from_signed_imm);
--   type c_source_type is (from_null, from_alu, from_shift, 
--      from_mult, from_memory, from_pc, from_imm_shift16,
--      from_reg_source_nez, from_reg_source_eqz);
--   type pc_source_type is (from_inc4, from_inc8, from_reg_source, 
--      from_opcode25_0, from_branch, from_lbranch);
begin

control_proc: process(opcode, intr_signal) 
   variable op, func       : std_logic_vector(5 downto 0);
   variable rs, rt, rd     : std_logic_vector(5 downto 0);
   variable rtx            : std_logic_vector(4 downto 0);
   variable imm            : std_logic_vector(15 downto 0);
   variable alu_function   : alu_function_type;
   variable shift_function : shift_function_type;
   variable mult_function  : mult_function_type;
   variable a_source       : a_source_type;
   variable b_source       : b_source_type;
   variable c_source       : c_source_type;
   variable pc_source      : pc_source_type;
   variable branch_function: branch_function_type;
   variable mem_source     : mem_source_type;
begin
   alu_function := alu_nothing;
   shift_function := shift_nothing;
   mult_function := mult_nothing;
   a_source := a_from_reg_source;
   b_source := b_from_reg_target;
   c_source := c_from_null;
   pc_source := from_inc4;
   branch_function := branch_eq;
   mem_source := mem_fetch;
   op := opcode(31 downto 26);
   rs := '0' & opcode(25 downto 21);
   rt := '0' & opcode(20 downto 16);
   rtx := opcode(20 downto 16);
   rd := '0' & opcode(15 downto 11);
   func := opcode(5 downto 0);
   imm := opcode(15 downto 0);

   case op is
   when "000000" =>   --SPECIAL
      case func is
      when "000000" =>   --SLL   r[rd]=r[rt]<<re;
         a_source := a_from_imm10_6;
         c_source := c_from_shift;
         shift_function := shift_left_unsigned;
      when "000010" =>   --SRL   r[rd]=u[rt]>>re;
         a_source := a_from_imm10_6;
         c_source := c_from_shift;
         shift_function := shift_right_unsigned;
      when "000011" =>   --SRA   r[rd]=r[rt]>>re;
         a_source := a_from_imm10_6;
         c_source := c_from_shift;
         shift_function := shift_right_signed;
      when "000100" =>   --SLLV  r[rd]=r[rt]<<r[rs];
         c_source := c_from_shift;
         shift_function := shift_left_unsigned;
      when "000110" =>   --SRLV  r[rd]=u[rt]>>r[rs];
         c_source := c_from_shift;
         shift_function := shift_right_unsigned;
      when "000111" =>   --SRAV  r[rd]=r[rt]>>r[rs];
         c_source := c_from_shift;
         shift_function := shift_right_signed;
      when "001000" =>   --JR    s->pc_next=r[rs];
         pc_source := from_branch;
         alu_function := alu_add;
         branch_function := branch_yes;
      when "001001" =>   --JALR  r[rd]=s->pc_next; s->pc_next=r[rs];
         c_source := c_from_pc_plus4;
         pc_source := from_branch;
         alu_function := alu_add;
         branch_function := branch_yes;
      when "001010" =>   --MOVZ  if(!r[rt]) r[rd]=r[rs]; /*IV*/
--         c_source := c_from_reg_source_eqz;
      when "001011" =>   --MOVN  if(r[rt]) r[rd]=r[rs];  /*IV*/
--         c_source := from_reg_source_nez;
      when "001100" =>   --SYSCALL
--         if(r[4]==0) printf("0x%8.8lx ",r[5]);
      when "001101" =>   --BREAK s->wakeup=1;
      when "001111" =>   --SYNC  s->wakeup=1;
      when "010000" =>   --MFHI  r[rd]=s->hi;
         c_source := c_from_mult;
         mult_function := mult_read_hi;
      when "010001" =>   --FTHI  s->hi=r[rs];
         mult_function := mult_write_hi;
      when "010010" =>   --MFLO  r[rd]=s->lo;
         c_source := c_from_mult;
         mult_function := mult_read_lo;
      when "010011" =>   --MTLO  s->lo=r[rs];
         mult_function := mult_write_lo;
      when "011000" =>   --MULT  s->lo=r[rs]*r[rt]; s->hi=0;
         mult_function := mult_signed_mult;
      when "011001" =>   --MULTU s->lo=r[rs]*r[rt]; s->hi=0;
         mult_function := mult_mult;
      when "011010" =>   --DIV   s->lo=r[rs]/r[rt]; s->hi=r[rs]%r[rt];
         mult_function := mult_signed_divide;
      when "011011" =>   --DIVU  s->lo=r[rs]/r[rt]; s->hi=r[rs]%r[rt];
         mult_function := mult_divide;
      when "100000" =>   --ADD   r[rd]=r[rs]+r[rt];
         c_source := c_from_alu;
         alu_function := alu_add;
      when "100001" =>   --ADDU  r[rd]=r[rs]+r[rt];
         c_source := c_from_alu;
         alu_function := alu_add;
      when "100010" =>   --SUB   r[rd]=r[rs]-r[rt];
         c_source := c_from_alu;
         alu_function := alu_subtract;
      when "100011" =>   --SUBU  r[rd]=r[rs]-r[rt];
         c_source := c_from_alu;
         alu_function := alu_subtract;
      when "100100" =>   --AND   r[rd]=r[rs]&r[rt];
         c_source := c_from_alu;
         alu_function := alu_and;
      when "100101" =>   --OR    r[rd]=r[rs]|r[rt];
         c_source := c_from_alu;
         alu_function := alu_or;
      when "100110" =>   --XOR   r[rd]=r[rs]^r[rt];
         c_source := c_from_alu;
         alu_function := alu_xor;
      when "100111" =>   --NOR   r[rd]=~(r[rs]|r[rt]);
         c_source := c_from_alu;
         alu_function := alu_nor;
      when "101010" =>   --SLT   r[rd]=r[rs]<r[rt];
         c_source := c_from_alu;
         alu_function := alu_less_than_signed;
      when "101011" =>   --SLTU  r[rd]=u[rs]<u[rt];
         c_source := c_from_alu;
         alu_function := alu_less_than;
      when "101101" =>   --DADDU r[rd]=r[rs]+u[rt];
         c_source := c_from_alu;
         alu_function := alu_add;
      when "110001" =>   --TGEU
      when "110010" =>   --TLT
      when "110011" =>   --TLTU
      when "110100" =>   --TEQ 
      when "110110" =>   --TNE 
      when others =>
      end case;
   when "000001" =>   --REGIMM
      rt := "000000";
      rd := "011111";
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_branch;
      branch_function := branch_gtz;
      --if(test) pc=pc+imm*4
      case rtx is
      when "10000" =>   --BLTZAL  r[31]=s->pc_next; branch=r[rs]<0;
         c_source := c_from_pc_plus4;
         branch_function := branch_ltz;
      when "00000" =>   --BLTZ    branch=r[rs]<0;
         branch_function := branch_ltz;
      when "10001" =>   --BGEZAL  r[31]=s->pc_next; branch=r[rs]>=0;
         c_source := c_from_pc_plus4;
         branch_function := branch_gez;
      when "00001" =>   --BGEZ    branch=r[rs]>=0;
         branch_function := branch_gez;
      when "10010" =>   --BLTZALL r[31]=s->pc_next; lbranch=r[rs]<0;
         c_source := c_from_pc_plus4;
         pc_source := from_lbranch;
         branch_function := branch_ltz;
      when "00010" =>   --BLTZL   lbranch=r[rs]<0;
         pc_source := from_lbranch;
         branch_function := branch_ltz;
      when "10011" =>   --BGEZALL r[31]=s->pc_next; lbranch=r[rs]>=0;
         c_source := c_from_pc_plus4;
         pc_source := from_lbranch;
         branch_function := branch_gez;
      when "00011" =>   --BGEZL   lbranch=r[rs]>=0;
         pc_source := from_lbranch;
         branch_function := branch_gez;
	  when others =>
	  end case;
   when "000011" =>   --JAL    r[31]=s->pc_next; s->pc_next=(s->pc&0xf0000000)|target;
      c_source := c_from_pc_plus4;
      rd := "011111";
      pc_source := from_opcode25_0;
   when "000010" =>   --J      s->pc_next=(s->pc&0xf0000000)|target; 
      pc_source := from_opcode25_0;
   when "000100" =>   --BEQ    branch=r[rs]==r[rt];
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_branch;
      branch_function := branch_eq;
   when "000101" =>   --BNE    branch=r[rs]!=r[rt];
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_branch;
      branch_function := branch_ne;
   when "000110" =>   --BLEZ   branch=r[rs]<=0;
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_branch;
      branch_function := branch_lez;
   when "000111" =>   --BGTZ   branch=r[rs]>0;
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_branch;
      branch_function := branch_gtz;
   when "001000" =>   --ADDI   r[rt]=r[rs]+(short)imm;
      b_source := b_from_signed_imm;
      c_source := c_from_alu;
      rd := rt;
      alu_function := alu_add;
   when "001001" =>   --ADDIU  u[rt]=u[rs]+(short)imm;
      b_source := b_from_signed_imm;
      c_source := c_from_alu;
      rd := rt;
      alu_function := alu_add;
   when "001010" =>   --SLTI   r[rt]=r[rs]<(short)imm;
      b_source := b_from_signed_imm;
      c_source := c_from_alu;
      rd := rt;
      alu_function := alu_less_than;
   when "001011" =>   --SLTIU  u[rt]=u[rs]<(unsigned long)(short)imm;
      b_source := b_from_imm;
      c_source := c_from_alu;
      rd := rt;
      alu_function := alu_less_than;
   when "001100" =>   --ANDI   r[rt]=r[rs]&imm;
      b_source := b_from_imm;
      c_source := c_from_alu;
      rd := rt;
      alu_function := alu_and;
   when "001101" =>   --ORI    r[rt]=r[rs]|imm;
      b_source := b_from_imm;
      c_source := c_from_alu;
      rd := rt;
      alu_function := alu_or;
   when "001110" =>   --XORI   r[rt]=r[rs]^imm;
      b_source := b_from_imm;
      c_source := c_from_alu;
      rd := rt;
      alu_function := alu_xor;
   when "001111" =>   --LUI    r[rt]=(imm<<16);
      c_source := c_from_imm_shift16;
      rd := rt;
   when "010000" =>   --COP0
      alu_function := alu_or;
      c_source := c_from_alu;
      if opcode(23) = '0' then  --move from CP0
         rs := '1' & opcode(15 downto 11);
         rt := "000000";
         rd := '0' & opcode(20 downto 16);
      else                      --move to CP0
         rs := "000000";
         rd(5) := '1';
      end if;
   when "010001" =>   --COP1
   when "010010" =>   --COP2
   when "010011" =>   --COP3
   when "010100" =>   --BEQL   lbranch=r[rs]==r[rt];
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_lbranch;
      branch_function := branch_eq;
   when "010101" =>   --BNEL   lbranch=r[rs]!=r[rt];
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_lbranch;
      branch_function := branch_ne;
   when "010110" =>   --BLEZL  lbranch=r[rs]<=0;
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_lbranch;
      branch_function := branch_lez;
   when "010111" =>   --BGTZL  lbranch=r[rs]>0;
      a_source := a_from_pc;
      b_source := b_from_immX4;
      alu_function := alu_add;
      pc_source := from_lbranch;
      branch_function := branch_gtz;
   when "100000" =>   --LB     r[rt]=*(signed char*)ptr;
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      rd := rt;
      c_source := c_from_memory;
      mem_source := mem_read8s;    --address=(short)imm+r[rs];
   when "100001" =>   --LH     r[rt]=*(signed short*)ptr;
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      rd := rt;
      c_source := c_from_memory;
      mem_source := mem_read16s;   --address=(short)imm+r[rs];
   when "100010" =>   --LWL    //Not Implemented
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      rd := rt;
      c_source := c_from_memory;
      mem_source := mem_read32;
   when "100011" =>   --LW     r[rt]=*(long*)ptr;
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      rd := rt;
      c_source := c_from_memory;
      mem_source := mem_read32;
   when "100100" =>   --LBU    r[rt]=*(unsigned char*)ptr;
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      rd := rt;
      c_source := c_from_memory;
      mem_source := mem_read8;    --address=(short)imm+r[rs];
   when "100101" =>   --LHU    r[rt]=*(unsigned short*)ptr;
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      rd := rt;
      c_source := c_from_memory;
      mem_source := mem_read16;    --address=(short)imm+r[rs];
   when "100110" =>   --LWR    //Not Implemented
   when "101000" =>   --SB     *(char*)ptr=(char)r[rt];
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      mem_source := mem_write8;   --address=(short)imm+r[rs];
   when "101001" =>   --SH     *(short*)ptr=(short)r[rt];
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      mem_source := mem_write16;
   when "101010" =>   --SWL    //Not Implemented
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      mem_source := mem_write32;  --address=(short)imm+r[rs];
   when "101011" =>   --SW     *(long*)ptr=r[rt];
      a_source := a_from_reg_source;
      b_source := b_from_imm;
      alu_function := alu_add;
      mem_source := mem_write32;  --address=(short)imm+r[rs];
   when "101110" =>   --SWR    //Not Implemented
   when "101111" =>   --CACHE
   when "110000" =>   --LL     r[rt]=*(long*)ptr;
   when "110001" =>   --LWC1 
   when "110010" =>   --LWC2 
   when "110011" =>   --LWC3 
   when "110101" =>   --LDC1 
   when "110110" =>   --LDC2 
   when "110111" =>   --LDC3 
   when "111000" =>   --SC     *(long*)ptr=r[rt]; r[rt]=1;
   when "111001" =>   --SWC1 
   when "111010" =>   --SWC2 
   when "111011" =>   --SWC3 
   when "111101" =>   --SDC1 
   when "111110" =>   --SDC2 
   when "111111" =>   --SDC3 
   when others =>
   end case;

   if c_source = c_from_null then
      rd := "000000";
   end if;

   if intr_signal = '1' then
      rs := "111111";  --interrupt vector
      rt := "000000";
      rd := "101110";  --save PC in EPC
      alu_function := alu_or;
      shift_function := shift_nothing;
      mult_function := mult_nothing;
      branch_function := branch_yes;
      a_source := a_from_reg_source;
      b_source := b_from_reg_target;
      c_source := c_from_pc;
      pc_source := from_lbranch;
      mem_source := mem_fetch;
   end if;

   rs_index <= rs;
   rt_index <= rt;
   rd_index <= rd;
   imm_out <= imm;
   alu_func <= alu_function;
   shift_func <= shift_function;
   mult_func <= mult_function;
   branch_func <= branch_function;
   a_source_out <= a_source;
   b_source_out <= b_source;
   c_source_out <= c_source;
   pc_source_out <= pc_source;
   mem_source_out <= mem_source;

end process;

end; --logic


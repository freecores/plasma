---------------------------------------------------------------------
-- TITLE: Arithmetic Logic Unit
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 2/8/01
-- FILENAME: alu.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Implements the ALU.
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use work.mlite_pack.all;

entity alu is
   generic(adder_type : string := "GENERIC";
           alu_type   : string := "GENERIC");
   port(a_in         : in  std_logic_vector(31 downto 0);
        b_in         : in  std_logic_vector(31 downto 0);
        alu_function : in  alu_function_type;
        c_alu        : out std_logic_vector(31 downto 0));
end; --alu

architecture logic of alu is
--   type alu_function_type is (alu_nothing, alu_add, alu_subtract, 
--      alu_less_than, alu_less_than_signed, 
--      alu_or, alu_and, alu_xor, alu_nor);
  
   signal aa, bb, sum : std_logic_vector(32 downto 0);
   signal do_add      : std_logic;
   signal sign_ext    : std_logic;
begin

   do_add <= '1' when alu_function = alu_add else '0';
   sign_ext <= '0' when alu_function = alu_less_than else '1';
   aa <= (a_in(31) and sign_ext) & a_in;
   bb <= (b_in(31) and sign_ext) & b_in;

   -- synthesis translate_off
   GENERIC_ALU: if alu_type="GENERIC" generate
   -- synthesis translate_on

      c_alu <= sum(31 downto 0) when alu_function=alu_add or alu_function=alu_subtract else
               ZERO(31 downto 1) & sum(32) when alu_function=alu_less_than or alu_function=alu_less_than_signed else
               a_in or  b_in    when alu_function=alu_or else
               a_in and b_in    when alu_function=alu_and else
               a_in xor b_in    when alu_function=alu_xor else
               a_in nor b_in    when alu_function=alu_nor else
               ZERO;

   -- synthesis translate_off
   end generate;
   -- synthesis translate_on

   -- synopsys synthesis_off

   AREA_OPTIMIZED_ALU: if alu_type="AREA_OPTIMIZED" generate
    
      c_alu <= sum(31 downto 0) when alu_function=alu_add or alu_function=alu_subtract else (others => 'Z');
      c_alu <= ZERO(31 downto 1) & sum(32) when alu_function=alu_less_than or alu_function=alu_less_than_signed else (others => 'Z');
      c_alu <= a_in or  b_in    when alu_function=alu_or else (others => 'Z');
      c_alu <= a_in and b_in    when alu_function=alu_and else (others => 'Z');
      c_alu <= a_in xor b_in    when alu_function=alu_xor else (others => 'Z');
      c_alu <= a_in nor b_in    when alu_function=alu_nor else (others => 'Z');
      c_alu <= ZERO             when alu_function=alu_nothing else (others => 'Z');
    
   end generate;
    
   generic_adder: if adder_type = "GENERIC" generate
      sum <= bv_adder(aa, bb, do_add);
   end generate; --generic_adder
    
   --For Altera
   lpm_adder: if adder_type = "ALTERA" generate
      lpm_add_sub_component : lpm_add_sub
      GENERIC MAP (
         lpm_width => 33,
         lpm_direction => "UNUSED",
         lpm_type => "LPM_ADD_SUB",
         lpm_hint => "ONE_INPUT_IS_CONSTANT=NO"
      )
      PORT MAP (
         dataa => aa,
         add_sub => do_add,
         datab => bb,
         result => sum
      );
   end generate; --lpm_adder

   -- synopsys synthesis_on

end; --architecture logic


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
   generic(adder_type : string := "GENERIC");
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
begin

alu_proc: process(a_in, b_in, alu_function, sum) 
   variable sign_ext  : std_logic;
begin
   if alu_function = alu_add then
      do_add <= '1';
   else
      do_add <= '0';
   end if;
   if alu_function = alu_less_than then
      sign_ext := '0';
   else
      sign_ext := '1';
   end if;
   aa <= (a_in(31) and sign_ext) & a_in;
   bb <= (b_in(31) and sign_ext) & b_in;

   case alu_function is
   when alu_add | alu_subtract => --c=a+b
      c_alu <= sum(31 downto 0);
   when alu_less_than =>          --c=a<b
      c_alu <= ZERO(31 downto 1) & sum(32);
   when alu_less_than_signed =>   --c=a<b;
      c_alu <= ZERO(31 downto 1) & sum(32);
   when alu_or =>                 --c=a|b
      c_alu <= a_in or b_in;
   when alu_and =>                --c=a&b
      c_alu <= a_in and b_in;
   when alu_xor =>                --c=a^b
      c_alu <= a_in xor b_in;
   when alu_nor =>                --c=~(a|b)
      c_alu <= a_in nor b_in;
   when others =>                 --alu_function = alu_nothing
      c_alu <= ZERO;
   end case;

end process;


   generic_adder:
   if adder_type /= "ALTERA" generate
      sum <= bv_adder(aa, bb, do_add);
   end generate; --generic_adder

   --For Altera
   lpm_adder: 
   if adder_type = "ALTERA" generate
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

end; --architecture logic


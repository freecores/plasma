---------------------------------------------------------------------
-- TITLE: Shifter Unit
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 2/2/01
-- FILENAME: shifter.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Implements the 32-bit shifter unit.
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use work.mlite_pack.all;

entity shifter is
   port(value        : in  std_logic_vector(31 downto 0);
        shift_amount : in  std_logic_vector(4 downto 0);
        shift_func   : in  shift_function_type;
        c_shift      : out std_logic_vector(31 downto 0));
end; --entity shifter

architecture logic of shifter is
--   type shift_function_type is (
--      shift_nothing, shift_left_unsigned, 
--      shift_left_signed, shift_right_unsigned);
begin

shift_proc: process(value, shift_amount, shift_func)  --barrel shifter unit
   variable shift1L, shift2L, shift4L, shift8L, shift16 : 
      std_logic_vector(31 downto 0);
   variable shift1R, shift2R, shift4R, shift8R : 
      std_logic_vector(31 downto 0);
   variable fills : std_logic_vector(31 downto 16);
variable go_right : std_logic;
begin
   if shift_func = shift_right_unsigned or shift_func = shift_right_signed then
      go_right := '1';
   else
      go_right := '0';
   end if;
   if shift_func = shift_right_signed and value(31) = '1' then
      fills := "1111111111111111";
   else
      fills := "0000000000000000";
   end if;
   if go_right = '0' then  --shift left
      if shift_amount(0) = '1' then
         shift1L := value(30 downto 0) & '0';
      else
         shift1L := value;
      end if;
      if shift_amount(1) = '1' then
         shift2L := shift1L(29 downto 0) & "00";
      else
         shift2L := shift1L;
      end if;
      if shift_amount(2) = '1' then
         shift4L := shift2L(27 downto 0) & "0000";
      else
         shift4L := shift2L;
      end if;
      if shift_amount(3) = '1' then
         shift8L := shift4L(23 downto 0) & "00000000";
      else
         shift8L := shift4L;
      end if;
      if shift_amount(4) = '1' then
         shift16 := shift8L(15 downto 0) & ZERO(15 downto 0);
      else
         shift16 := shift8L;
      end if;
   else  --shift right
      if shift_amount(0) = '1' then
         shift1R := fills(31) & value(31 downto 1);
      else
         shift1R := value;
      end if;
      if shift_amount(1) = '1' then
         shift2R := fills(31 downto 30) & shift1R(31 downto 2);
      else
         shift2R := shift1R;
      end if;
      if shift_amount(2) = '1' then
         shift4R := fills(31 downto 28) & shift2R(31 downto 4);
      else
         shift4R := shift2R;
      end if;
      if shift_amount(3) = '1' then
         shift8R := fills(31 downto 24) & shift4R(31 downto 8);
      else
         shift8R := shift4R;
      end if;
      if shift_amount(4) = '1' then
         shift16 := fills(31 downto 16) & shift8R(31 downto 16);
      else
         shift16 := shift8R;
      end if;
   end if;  --shift_dir
   if shift_func = shift_nothing then
      c_shift <= ZERO;
   else
      c_shift <= shift16;
   end if;
end process;

end; --architecture logic


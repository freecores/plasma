---------------------------------------------------------------------
-- TITLE: Multiplication and Division Unit
-- AUTHOR: Steve Rhoads (rhoadss@yahoo.com)
-- DATE CREATED: 1/31/01
-- FILENAME: mult.vhd
-- PROJECT: Plasma CPU core
-- COPYRIGHT: Software placed into the public domain by the author.
--    Software 'as is' without warranty.  Author liable for nothing.
-- DESCRIPTION:
--    Implements the multiplication and division unit.
--    Division takes 32 clock cycles.
--    Multiplication normally takes 16 clock cycles.
--    if b <= 0xffff then mult in 8 cycles. 
--    if b <= 0xff then mult in 4 cycles. 
--
-- For multiplication set reg_b = 0x00000000 & b.  The 64-bit result
-- will be in reg_b.  The lower bits of reg_b contain the upper 
-- bits of b that have not yet been multiplied.  For 16 clock cycles
-- shift reg_b two bits to the right.  Use the lowest two bits of reg_b 
-- to multiply by two bits at a time and add the result to the upper
-- 32-bits of reg_b (using C syntax):
--    reg_b = (reg_b >> 2) + (((reg_b & 3) * reg_a) << 32);
--
-- For division set reg_b = '0' & b & 30_ZEROS.  The answer will be
-- in answer_reg and the remainder in reg_a.  For 32 clock cycles
-- (using C syntax):
--    answer_reg = (answer_reg << 1);
--    if (reg_a >= reg_b) {
--       answer_reg += 1;
--       reg_a -= reg_b;
--    }
--    reg_b = reg_b >> 1;
---------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use work.mlite_pack.all;

entity mult is
   generic(adder_type : string := "GENERIC";
           mult_type  : string := "GENERIC");
   port(clk       : in std_logic;
        a, b      : in std_logic_vector(31 downto 0);
        mult_func : in mult_function_type;
        c_mult    : out std_logic_vector(31 downto 0);
        pause_out : out std_logic);
end; --entity mult

architecture logic of mult is
--   type mult_function_type is (
--      mult_nothing, mult_read_lo, mult_read_hi, mult_write_lo, 
--      mult_write_hi, mult_mult, mult_divide, mult_signed_divide);
   signal do_mult_reg   : std_logic;
   signal do_signed_reg : std_logic;
   signal count_reg     : std_logic_vector(5 downto 0);
   signal reg_a         : std_logic_vector(31 downto 0);
   signal reg_b         : std_logic_vector(63 downto 0);
   signal answer_reg    : std_logic_vector(31 downto 0);
   signal aa, bb        : std_logic_vector(33 downto 0);
   signal sum           : std_logic_vector(33 downto 0);
   signal reg_a_times3  : std_logic_vector(33 downto 0);
begin

--multiplication/division unit
mult_proc: process(clk, a, b, mult_func,
                   do_mult_reg, do_signed_reg, count_reg,
                   reg_a, reg_b, answer_reg, sum, reg_a_times3)
   variable do_mult_temp   : std_logic;
   variable do_signed_temp : std_logic;
   variable count_temp     : std_logic_vector(5 downto 0);
   variable a_temp         : std_logic_vector(31 downto 0);
   variable b_temp         : std_logic_vector(63 downto 0);
   variable answer_temp    : std_logic_vector(31 downto 0);
   variable start          : std_logic;
   variable do_write       : std_logic;
   variable do_hi          : std_logic;
   variable sign_extend    : std_logic;
   variable a_neg          : std_logic_vector(31 downto 0);
   variable b_neg          : std_logic_vector(31 downto 0);

begin
   do_mult_temp   := do_mult_reg;
   do_signed_temp := do_signed_reg;
   count_temp     := count_reg;
   a_temp         := reg_a;
   b_temp         := reg_b;
   answer_temp    := answer_reg;
   sign_extend    := do_signed_reg and do_mult_reg;
   start          := '0';
   do_write       := '0';
   do_hi          := '0';

   case mult_func is
   when mult_read_lo =>
   when mult_read_hi =>
      do_hi := '1';
   when mult_write_lo =>
      do_write := '1';
   when mult_write_hi =>
      do_write := '1';
      do_hi := '1';
   when mult_mult =>
      start := '1';
      do_mult_temp := '1';
      do_signed_temp := '0';
   when mult_signed_mult =>
      start := '1';
      do_mult_temp := '1';
      do_signed_temp := '1';
   when mult_divide =>
      start := '1';
      do_mult_temp := '0';
      do_signed_temp := '0';
   when mult_signed_divide =>
      start := '1';
      do_mult_temp := '0';
      do_signed_temp := a(31) xor b(31);
   when others =>
   end case;

   if start = '1' then
      count_temp := "000000";
      answer_temp := ZERO;
      a_neg := bv_negate(a);
      b_neg := bv_negate(b);
      if do_mult_temp = '0' then
         b_temp(63) := '0';
         if mult_func /= mult_signed_divide or a(31) = '0' then
            a_temp := a;
         else
            a_temp := a_neg;
         end if;
         if mult_func /= mult_signed_divide or b(31) = '0' then
            b_temp(62 downto 31) := b;
         else
            b_temp(62 downto 31) := b_neg;
         end if;
         b_temp(30 downto 0) := ZERO(30 downto 0);
      else --multiply
         if do_signed_temp = '0' or b(31) = '0' then
            a_temp := a;
            b_temp(31 downto 0) := b;
         else
            a_temp := a_neg;
            b_temp(31 downto 0) := b_neg;
         end if;
         b_temp(63 downto 32) := ZERO;
      end if;
   elsif do_write = '1' then
      if do_hi = '0' then
         b_temp(31 downto 0) := a;
      else
         b_temp(63 downto 32) := a;
      end if;
   end if;

   if do_mult_reg = '0' then  --division
      aa <= (reg_a(31) and sign_extend) & (reg_a(31) and sign_extend) & reg_a;
      bb <= reg_b(33 downto 0);
   else                       --multiplication two-bits at a time
      case reg_b(1 downto 0) is
      when "00" =>
         aa <= "00" & ZERO;
      when "01" =>
         aa <= (reg_a(31) and sign_extend) & (reg_a(31) and sign_extend) & reg_a;
      when "10" =>
         aa <= (reg_a(31) and sign_extend) & reg_a & '0';
      when others =>
         aa <= reg_a_times3;
      end case;
      bb <= (reg_b(63) and sign_extend) & (reg_b(63) and sign_extend) & reg_b(63 downto 32);
   end if;

   if count_reg(5) = '0' and start = '0' then
      count_temp := bv_inc6(count_reg);
      if do_mult_reg = '0' then          --division
         answer_temp(31 downto 1) := answer_reg(30 downto 0);
         if reg_b(63 downto 32) = ZERO and sum(32) = '0' then
            a_temp := sum(31 downto 0);  --aa=aa-bb;
            answer_temp(0) := '1';
         else
            answer_temp(0) := '0';
         end if;
         if count_reg /= "011111" then
            b_temp(62 downto 0) := reg_b(63 downto 1);
         else                            --done with divide
            b_temp(63 downto 32) := a_temp;
            if do_signed_reg = '0' then
               b_temp(31 downto 0) := answer_temp;
            else
               b_temp(31 downto 0) := bv_negate(answer_temp);
            end if;
         end if;
      else  -- mult_mode
         b_temp(63 downto 30) := sum;
         b_temp(29 downto 0) := reg_b(31 downto 2);
         if count_reg = "001000" and sign_extend = '0' and   --early stop
               reg_b(15 downto 0) = ZERO(15 downto 0) then
            count_temp := "111111";
            b_temp(31 downto 0) := reg_b(47 downto 16);
         end if;
         if count_reg = "000100" and sign_extend = '0' and   --early stop
               reg_b(23 downto 0) = ZERO(23 downto 0) then
            count_temp := "111111";
            b_temp(31 downto 0) := reg_b(55 downto 24);
         end if;
         count_temp(5) := count_temp(4);
      end if;
   end if;

   if rising_edge(clk) then
      do_mult_reg <= do_mult_temp;
      do_signed_reg <= do_signed_temp;
      count_reg <= count_temp;
      reg_a <= a_temp;
      reg_b <= b_temp;
      answer_reg <= answer_temp;
      if start = '1' then
         reg_a_times3 <= ((a_temp(31) and do_signed_temp) & a_temp & '0') +
            ((a_temp(31) and do_signed_temp) & (a_temp(31) and do_signed_temp) & a_temp);
      end if;
   end if;

   if count_reg(5) = '0' and mult_func/= mult_nothing and start = '0' then
      pause_out <= '1';
   else
      pause_out <= '0';
   end if;
   case mult_func is
   when mult_read_lo =>
      c_mult <= reg_b(31 downto 0);
   when mult_read_hi =>
      c_mult <= reg_b(63 downto 32);
   when others =>
      c_mult <= ZERO;
   end case;

end process;


   generic_adder:
   if adder_type = "GENERIC" generate
      sum <= (aa + bb) when do_mult_reg = '1' else
             (aa - bb);
   end generate; --generic_adder

   --For Altera
   lpm_adder: 
   if adder_type = "ALTERA" generate
      lpm_add_sub_component : lpm_add_sub
      GENERIC MAP (
         lpm_width => 34,
         lpm_direction => "UNUSED",
         lpm_type => "LPM_ADD_SUB",
         lpm_hint => "ONE_INPUT_IS_CONSTANT=NO"
      )
      PORT MAP (
         dataa => aa,
         add_sub => do_mult_reg,
         datab => bb,
         result => sum
      );
   end generate; --lpm_adder

end; --architecture logic


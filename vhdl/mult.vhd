---------------------------------------------------------------------
-- TITLE: Multiplication and Division Unit
-- AUTHORS: Steve Rhoads (rhoadss@yahoo.com)
--          Matthias Gruenewald
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
--library ieee, MLITE_LIB;
--use MLITE_LIB.all;
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_unsigned.all;
use IEEE.std_logic_arith.all;
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
   signal sum2          : std_logic_vector(67 downto 0);
   signal reg_a_times3  : std_logic_vector(33 downto 0);
   signal sign_extend_sig : std_logic;

   --Used in Xilinx tri-state area optimizated version
   signal SUB_Y, A_PROCESSED, B_PROCESSED, A_REG, B_REG : std_logic_vector(31 downto 0);
   signal DIV_Y, DIV_Y_IN, DIV_Y_IN_CALC, DIV_Y_IN_INIT, Y_IN, Y_IN2, MULT_Y, Y : std_logic_vector(63 downto 0) := (others => '0');
   signal SAVE_Y, SAVE_DIV_Y, MULT_ND, MULT_RDY, DO_SIGNED, DIV_ND : std_logic;
   signal DIV_COUNT, INVERT_A, INVERT_B, INVERT_Y, DIV_RDY : std_logic;
   signal DIV_COUNTER : std_logic_vector(4 downto 0);
   signal PAUSE_IN, SAVE_PAUSE, PAUSE : std_logic := '0';
   signal MULT_RFD : std_logic;
   signal a_temp_sig, a_neg_sig, b_neg_sig : std_logic_vector(31 downto 0);
   signal b_temp_sig    : std_logic_vector(63 downto 0);
   signal a_msb, b_msb  : std_logic;
   signal answer_temp_sig : std_logic_vector(31 downto 0);
   signal aa_select     : std_logic_vector(3 downto 0);
   signal bb_select     : std_logic_vector(1 downto 0);
   signal a_select      : std_logic_vector(4 downto 0);
   signal b_select      : std_logic_vector(11 downto 0);
   signal answer_select : std_logic_vector(2 downto 0);
  
begin
 
   --sum = aa + bb
   generic_adder: if adder_type = "GENERIC" generate
      sum <= (aa + bb) when do_mult_reg = '1' else
             (aa - bb);
   end generate; --generic_adder

   --For Altera: sum = aa + bb
   lpm_adder: if adder_type = "ALTERA" generate
      lpm_add_sub_component : lpm_add_sub
        GENERIC MAP (
          lpm_width => 34,
          lpm_direction => "UNUSED",
          lpm_type => "LPM_ADD_SUB",
          lpm_hint => "ONE_INPUT_IS_CONSTANT=NO"
          )
        PORT MAP (
          dataa   => aa,
          add_sub => do_mult_reg,
          datab   => bb,
          result  => sum
          );
   end generate; --lpm_adder

   -- Negate signals
   a_neg_sig <= bv_negate(a);
   b_neg_sig <= bv_negate(b);
   sign_extend_sig <= do_signed_reg and do_mult_reg;
    
   -- Result
   c_mult <= reg_b(31 downto 0)  when mult_func=mult_read_lo else 
             reg_b(63 downto 32) when mult_func=mult_read_hi else 
             ZERO;


   GENERIC_MULT: if MULT_TYPE="GENERIC" generate
    
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
         if do_mult_temp = '0' then
            b_temp(63) := '0';
            if mult_func /= mult_signed_divide or a(31) = '0' then
               a_temp := a;
            else
               a_temp := a_neg_sig;
            end if;
            if mult_func /= mult_signed_divide or b(31) = '0' then
               b_temp(62 downto 31) := b;
            else
               b_temp(62 downto 31) := b_neg_sig;
            end if;
            b_temp(30 downto 0) := ZERO(30 downto 0);
         else --multiply
            if do_signed_temp = '0' or b(31) = '0' then
               a_temp := a;
               b_temp(31 downto 0) := b;
            else
               a_temp := a_neg_sig;
               b_temp(31 downto 0) := b_neg_sig;
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

      if count_reg(5) = '0' and mult_func /= mult_nothing and start = '0' then
         pause_out <= '1';
      else
         pause_out <= '0';
      end if;
      
   end process;

   end generate;


   AREA_OPTIMIZED_MULT: if MULT_TYPE="AREA_OPTIMIZED" generate
   --Xilinx Tristate size optimization by Matthias Gruenewald
    
   --multiplication/division unit
    mult_proc: process(a, b, clk, count_reg, do_mult_reg, do_signed_reg, mult_func, reg_b, sum)
      variable do_mult_temp   : std_logic;
      variable do_signed_temp : std_logic;
      variable count_temp     : std_logic_vector(5 downto 0);
      variable start          : std_logic;
      variable do_write       : std_logic;
      variable do_hi          : std_logic;
      variable sign_extend    : std_logic;

    begin
      do_mult_temp   := do_mult_reg;
      do_signed_temp := do_signed_reg;
      count_temp     := count_reg;
      sign_extend    := do_signed_reg and do_mult_reg;
      sign_extend_sig <= sign_extend;
      start          := '0';
      do_write       := '0';
      do_hi          := '0';
      a_select <= (others => '0');
      b_select <= (others => '0');
      aa_select <= (others => '0');
      bb_select <= (others => '0');
      answer_select <= (others => '0');
      
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
        answer_select(0)<='1';
        --answer_temp := ZERO;
        if do_mult_temp = '0' then
          --b_temp(63) := '0';
          if mult_func /= mult_signed_divide or a(31) = '0' then
            a_select(0) <= '1';
            --a_temp := a;
          else
            a_select(1) <= '1';
            --a_temp := a_neg;
          end if;
          if mult_func /= mult_signed_divide or b(31) = '0' then
            b_select(0) <= '1';
            --b_temp(62 downto 31) := b;
          else
            b_select(1) <= '1';
            --b_temp(62 downto 31) := b_neg;
          end if;
          --b_temp(30 downto 0) := ZERO(30 downto 0);
        else --multiply
          if do_signed_temp = '0' or b(31) = '0' then
            a_select(2) <= '1';
            --a_temp := a;
            b_select(2) <= '1';
            --b_temp(31 downto 0) := b;
          else
            a_select(3) <= '1';
            --a_temp := a_neg;
            b_select(3) <= '1';
            --b_temp(31 downto 0) := b_neg;
          end if;
          --b_temp(63 downto 32) := ZERO;
        end if;
      elsif do_write = '1' then
        if do_hi = '0' then
          b_select(4) <= '1';
          --b_temp(31 downto 0) := a;
        else
          b_select(5) <= '1';
          --b_temp(63 downto 32) := a;
        end if;
      end if;

      if do_mult_reg = '0' then  --division
        aa_select(0) <= '1';
        --aa <= (reg_a(31) and sign_extend) & (reg_a(31) and sign_extend) & reg_a;
        bb_select(0) <= '1';
        --bb <= reg_b(33 downto 0);
      else                       --multiplication two-bits at a time
        case reg_b(1 downto 0) is
          when "00" =>
            aa_select(1) <= '1';
            --aa <= "00" & ZERO;
          when "01" =>
            aa_select(2) <= '1';
            --aa <= (reg_a(31) and sign_extend) & (reg_a(31) and sign_extend) & reg_a;
          when "10" =>
            aa_select(3) <= '1';
            --aa <= (reg_a(31) and sign_extend) & reg_a & '0';
          when others =>
            --aa_select(4) <= '1';
            --aa <= reg_a_times3;
        end case;
        bb_select(1) <= '1';
        --bb <= (reg_b(63) and sign_extend) & (reg_b(63) and sign_extend) & reg_b(63 downto 32);
      end if;

      if count_reg(5) = '0' and start = '0' then
        count_temp := bv_inc6(count_reg);
        if do_mult_reg = '0' then          --division          
          --answer_temp(31 downto 1) := answer_reg(30 downto 0);
          if reg_b(63 downto 32) = ZERO and sum(32) = '0' then
            a_select(4) <= '1';
            --a_temp := sum(31 downto 0);  --aa=aa-bb;
            answer_select(1) <= '1';
            --answer_temp(0) := '1';
          else
            answer_select(2) <= '1';
            --answer_temp(0) := '0';
          end if;
          if count_reg /= "011111" then
            --b_temp(62 downto 0) := reg_b(63 downto 1);
            b_select(6) <= '1';
          else                            --done with divide
            --b_temp(63 downto 32) := a_temp;
            if do_signed_reg = '0' then
              b_select(7) <= '1';
              --b_temp(31 downto 0) := answer_temp;
            else
              b_select(8) <= '1';
              --b_temp(31 downto 0) := bv_negate(answer_temp);
            end if;
          end if;
        else  -- mult_mode
          b_select(9) <= '1';
          --b_temp(63 downto 30) := sum;
          --b_temp(29 downto 0) := reg_b(31 downto 2);
          if count_reg = "001000" and sign_extend = '0' and   --early stop
            reg_b(15 downto 0) = ZERO(15 downto 0) then
            count_temp := "111111";
            b_select(10) <= '1';
            --b_temp(31 downto 0) := reg_b(47 downto 16);
          end if;
          if count_reg = "000100" and sign_extend = '0' and   --early stop
            reg_b(23 downto 0) = ZERO(23 downto 0) then
            count_temp := "111111";
            b_select(11) <= '1';
            --b_temp(31 downto 0) := reg_b(55 downto 24);
          end if;
          count_temp(5) := count_temp(4);
        end if;
      end if;

      if rising_edge(clk) then
        do_mult_reg <= do_mult_temp;
        do_signed_reg <= do_signed_temp;
        count_reg <= count_temp;
        reg_a <= a_temp_sig;
        reg_b <= b_temp_sig;
        answer_reg <= answer_temp_sig;
        if start = '1' then
          reg_a_times3 <= ((a_temp_sig(31) and do_signed_temp) & a_temp_sig & '0') +
                          ((a_temp_sig(31) and do_signed_temp) & (a_temp_sig(31) and do_signed_temp) & a_temp_sig);
        end if;
      end if;

      if count_reg(5) = '0' and mult_func/= mult_nothing and start = '0' then
        pause_out <= '1';
      else
        pause_out <= '0';
      end if;
      
    end process;


    -- Arguments
    a_msb <= reg_a(31) and sign_extend_sig;
    aa <= a_msb & a_msb & reg_a when aa_select(0)='1' else 
          "00" & ZERO           when aa_select(1)='1' else
          a_msb & a_msb & reg_a when aa_select(2)='1' else
          a_msb & reg_a & '0'   when aa_select(3)='1' else
          reg_a_times3;                                                              

    b_msb <= reg_b(63) and sign_extend_sig;
    bb <= reg_b(33 downto 0)                  when bb_select(0)='1' else (others => 'Z');
    bb <= b_msb & b_msb & reg_b(63 downto 32) when bb_select(1)='1' else (others => 'Z');

    -- Divide: Init
    a_temp_sig <= a                                   when a_select(0)='1' else (others => 'Z');
    a_temp_sig <= a_neg_sig                           when a_select(1)='1' else (others => 'Z');    
    b_temp_sig <= '0' & b & ZERO(30 downto 0)         when b_select(0)='1' else (others => 'Z');
    b_temp_sig <= '0' & b_neg_sig & ZERO(30 downto 0) when b_select(1)='1' else (others => 'Z');
    
    -- Multiply: Init
    a_temp_sig <= a                when a_select(2)='1' else (others => 'Z');
    b_temp_sig <= ZERO & b         when b_select(2)='1' else (others => 'Z');
    a_temp_sig <= a_neg_sig        when a_select(3)='1' else (others => 'Z');
    b_temp_sig <= ZERO & b_neg_sig when b_select(3)='1' else (others => 'Z');

    -- Intermediate results
    b_temp_sig  <= reg_b(63 downto 32) & a when b_select(4)='1' else (others => 'Z');
    b_temp_sig  <= a & reg_b(31 downto 0)  when b_select(5)='1' else (others => 'Z');

    -- Divide: Operation
    a_temp_sig <= sum(31 downto 0)                        when a_select(4)='1'      else (others => 'Z');
    b_temp_sig <= reg_b(63) & reg_b(63 downto 1)          when b_select(6)='1'      else (others => 'Z');
    b_temp_sig <= a_temp_sig & answer_temp_sig            when b_select(7)='1'      else (others => 'Z');
    b_temp_sig <= a_temp_sig & bv_negate(answer_temp_sig) when b_select(8)='1'      else (others => 'Z');

    -- Multiply: Operation
    b_temp_sig <= sum & reg_b(31 downto 2)               when b_select(9)='1' and b_select(10)='0' and b_select(11)='0' else (others => 'Z');
    b_temp_sig <= sum(33 downto 2) & reg_b(47 downto 16) when b_select(10)='1'                                          else (others => 'Z');
    b_temp_sig <= sum(33 downto 2) & reg_b(55 downto 24) when b_select(11)='1'                                          else (others => 'Z');

    -- Default values
    a_temp_sig <= reg_a           when conv_integer(unsigned(a_select))=0 else (others => 'Z');
    b_temp_sig <= reg_b           when conv_integer(unsigned(b_select))=0 else (others => 'Z');

    -- Result
    answer_temp_sig <= ZERO                          when answer_select(0)='1' else
                       answer_reg(30 downto 0) & '1' when answer_select(1)='1' else
                       answer_reg(30 downto 0) & '0' when answer_select(2)='1' else
                       answer_reg;
    
  end generate;
    
end; --architecture logic

